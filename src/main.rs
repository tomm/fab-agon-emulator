use crate::parse_args::parse_args;
use agon_ez80_emulator::debugger;
use agon_ez80_emulator::debugger::{DebugCmd, DebugResp, DebuggerConnection, Trigger};
use agon_ez80_emulator::{gpio, AgonMachine, AgonMachineConfig, RamInit, SerialLink};
use sdl2::event::Event;
use std::sync::mpsc;
use std::sync::mpsc::{Receiver, Sender};
use std::sync::{Arc, Mutex};
use std::thread;
mod ascii2vk;
mod audio;
mod ez80_serial_links;
mod joypad;
mod parse_args;
mod sdl2ps2;
mod vdp_interface;

const AUDIO_BUFLEN: u16 = 256;
const PREFIX: Option<&'static str> = option_env!("PREFIX");

/**
 * Return firmware paths in priority order.
 */
pub fn firmware_paths(
    ver: parse_args::FirmwareVer,
    explicit_path: Option<std::path::PathBuf>,
    is_mos: bool,
) -> Vec<std::path::PathBuf> {
    let mut paths: Vec<std::path::PathBuf> = vec![];

    if let Some(ref p) = explicit_path {
        paths.push(p.clone());
    }

    let base_path = match PREFIX {
        None => std::path::Path::new(".").join("firmware"),
        Some(prefix) => std::path::Path::new(prefix)
            .join("share")
            .join("fab-agon-emulator"),
    };

    for v in [ver, parse_args::FirmwareVer::console8] {
        paths.push(base_path.join(format!(
            "{}_{:?}.{}",
            if is_mos { "mos" } else { "vdp" },
            v,
            if is_mos { "bin" } else { "so" }
        )));
    }

    paths
}

pub fn main() -> () {
    std::process::exit(main_loop());
}

pub fn main_loop() -> i32 {
    let args = match parse_args() {
        Ok(a) => a,
        Err(e) => {
            eprintln!("Error parsing arguments: {}", e);
            std::process::exit(-1);
        }
    };
    let vdp_interface = vdp_interface::init(
        firmware_paths(args.firmware, args.vdp_dll, false),
        args.verbose,
    );

    unsafe { (*vdp_interface.setVdpDebugLogging)(args.verbose) }

    let (tx_cmd_debugger, rx_cmd_debugger): (Sender<DebugCmd>, Receiver<DebugCmd>) =
        mpsc::channel();
    let (tx_resp_debugger, rx_resp_debugger): (Sender<DebugResp>, Receiver<DebugResp>) =
        mpsc::channel();

    let gpios = Arc::new(Mutex::new(gpio::GpioSet::new()));

    // Atomics for various state communication
    let ez80_paused = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    let emulator_shutdown = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    let soft_reset = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    let exit_status = std::sync::Arc::new(std::sync::atomic::AtomicI32::new(0));

    for breakpoint in &args.breakpoints {
        let trigger = Trigger {
            address: *breakpoint,
            once: false,
            actions: vec![
                DebugCmd::Pause(debugger::PauseReason::DebuggerBreakpoint),
                DebugCmd::GetState,
            ],
        };
        let debug_cmd = DebugCmd::AddTrigger(trigger);
        _ = tx_cmd_debugger.send(debug_cmd);
    }

    // Preserve stdin state, as debugger can leave stdin in raw mode
    #[cfg(target_os = "linux")]
    let _tty = raw_tty::TtyWithGuard::new(std::io::stdin());

    let debugger_con = if args.debugger {
        let _ez80_paused = ez80_paused.clone();
        let _emulator_shutdown = emulator_shutdown.clone();
        let _debugger_thread = thread::spawn(move || {
            agon_light_emulator_debugger::start(
                tx_cmd_debugger,
                rx_resp_debugger,
                _emulator_shutdown,
                _ez80_paused.load(std::sync::atomic::Ordering::Relaxed),
            );
        });
        Some(DebuggerConnection {
            tx: tx_resp_debugger,
            rx: rx_cmd_debugger,
        })
    } else {
        None
    };

    let sdcard_img_file =
        args.sdcard_img.as_ref().and_then(|filename| {
            match std::fs::File::options()
                .read(true)
                .write(true)
                .open(filename)
            {
                Ok(file) => Some(file),
                Err(e) => {
                    eprintln!("Could not open sdcard image '{}': {:?}", filename, e);
                    std::process::exit(-1);
                }
            }
        });

    let _cpu_thread = {
        let _exit_status = exit_status.clone();
        let _ez80_paused = ez80_paused.clone();
        let _emulator_shutdown = emulator_shutdown.clone();
        let soft_reset_ez80 = soft_reset.clone();
        let gpios_ = gpios.clone();

        thread::Builder::new()
            .name("ez80".to_string())
            .spawn(move || {
                let ez80_firmware = firmware_paths(args.firmware, args.mos_bin, true).remove(0);

                let sdcard_dir = if let Some(p) = args.sdcard {
                    std::path::PathBuf::from(p)
                } else if let Some(home_dir) = home::home_dir() {
                    let p = home_dir.join(".agon-sdcard");
                    if p.exists() {
                        p
                    } else {
                        std::path::PathBuf::from("sdcard".to_string())
                    }
                } else {
                    std::path::PathBuf::from("sdcard".to_string())
                };

                if args.verbose {
                    eprintln!("EZ80 firmware: {:?}", ez80_firmware);
                    eprintln!("Emulated SDCard: {:?}", sdcard_dir);
                }

                let uart1_serial: Option<Box<dyn SerialLink>> =
                    args.uart1_device.as_ref().and_then(|device| {
                        let baud = args.uart1_baud.unwrap_or(9600);
                        println!("Using uart1 device {:?}, baud rate {:?}", device, baud);
                        match ez80_serial_links::Ez80ToHostSerialLink::try_open(&device, baud) {
                            Some(link) => Some(Box::new(link) as Box<dyn SerialLink>),
                            None => None,
                        }
                    });
                let uart1_dummy: Box<dyn SerialLink> =
                    Box::new(ez80_serial_links::DummySerialLink {
                        name: "uart1".to_string(),
                    });

                // Prepare the device
                let mut machine = AgonMachine::new(AgonMachineConfig {
                    ram_init: if args.zero {
                        RamInit::Zero
                    } else {
                        RamInit::Random
                    },
                    uart0_link: Box::new(ez80_serial_links::Ez80ToVdpSerialLink {
                        z80_uart0_is_cts: vdp_interface.z80_uart0_is_cts.clone(),
                        z80_send_to_vdp: vdp_interface.z80_send_to_vdp.clone(),
                        z80_recv_from_vdp: vdp_interface.z80_recv_from_vdp.clone(),
                    }),
                    uart1_link: uart1_serial.unwrap_or(uart1_dummy),
                    gpios: gpios_,
                    soft_reset: soft_reset_ez80,
                    emulator_shutdown: _emulator_shutdown,
                    exit_status: _exit_status,
                    paused: _ez80_paused,
                    clockspeed_hz: if args.unlimited_cpu {
                        1000_000_000
                    } else {
                        18_432_000
                    },
                    mos_bin: ez80_firmware,
                });
                machine.set_sdcard_directory(sdcard_dir);
                machine.set_sdcard_image(sdcard_img_file);
                machine.start(debugger_con);
                panic!("ez80 cpu thread terminated");
            })
    };

    // VDP thread
    let _vdp_thread = thread::Builder::new()
        .name("VDP".to_string())
        .spawn(move || unsafe {
            if let Some(scr_mode) = args.scr_mode {
                (*vdp_interface.set_startup_screen_mode)(scr_mode);
            }
            (*vdp_interface.vdp_setup)();
            (*vdp_interface.vdp_loop)();
        });

    let sdl_context = sdl2::init().unwrap();
    let native_resolution = sdl_context
        .video()
        .unwrap()
        .current_display_mode(0)
        .unwrap();
    let video_subsystem = sdl_context.video().unwrap();
    if args.osk {
        video_subsystem.text_input().start();
    }
    let joystick_subsystem = sdl_context.joystick().unwrap();
    let mut event_pump = sdl_context.event_pump().unwrap();
    let mut joysticks = vec![];

    open_joystick_devices(&mut joysticks, &joystick_subsystem);

    //println!("Detected {}x{} native resolution", native_resolution.w, native_resolution.h);

    let _audio_device = {
        match sdl_context.audio() {
            Ok(audio_subsystem) => {
                let desired_spec = sdl2::audio::AudioSpecDesired {
                    freq: Some(16384), // real VDP uses 16384Hz
                    channels: Some(1),
                    samples: Some(AUDIO_BUFLEN),
                };

                match audio_subsystem.open_playback(None, &desired_spec, |_spec| {
                    audio::VdpAudioStream {
                        getAudioSamples: vdp_interface.getAudioSamples,
                    }
                }) {
                    Ok(audio_device) => {
                        // start playback
                        audio_device.resume();

                        Some(audio_device)
                    }
                    Err(e) => {
                        println!("Error opening audio device: {:?}", e);
                        None
                    }
                }
            }
            Err(e) => {
                println!("Error opening audio subsystem: {:?}", e);
                None
            }
        }
    };

    let mut screen_scale = args.screen_scale;
    let mut is_fullscreen = args.fullscreen;
    // large enough for any agon video mode
    let mut vgabuf: Vec<u8> = Vec::with_capacity(1024 * 768 * 3);
    unsafe {
        vgabuf.set_len(1024 * 768 * 3);
    }
    let mut frame_rate_hz: f32 = 60.0;
    let mut mode_w: u32 = 640;
    let mut mode_h: u32 = 480;
    let mut mouse_btn_state: u8 = 0;

    joypad::clear_state(&mut gpios.lock().unwrap());

    'running: loop {
        let (wx, wy): (u32, u32) = {
            let nat = (native_resolution.w as u32, native_resolution.h as u32);
            if is_fullscreen {
                nat
            } else if nat.1 - 64 >= mode_h {
                // pick an integer scale that fits screen height (minus 64, to give titlebar space)
                calc_int_scale((nat.0, nat.1 - 64), (mode_w, mode_h))
            } else {
                // fall back to a window at least as large as the agon startup resolution (640x480)
                (mode_w, mode_h)
            }
        };

        sdl_context.mouse().set_relative_mouse_mode(is_fullscreen);

        let mut window = video_subsystem
            .window(
                &format!("Fab Agon Emulator {}", env!("CARGO_PKG_VERSION")),
                wx,
                wy,
            )
            .resizable()
            .position_centered()
            .build()
            .unwrap();

        if is_fullscreen {
            window
                .set_fullscreen(sdl2::video::FullscreenType::True)
                .unwrap();
        }

        let mut canvas = {
            match args.renderer {
                parse_args::Renderer::Software => window.into_canvas().software(),
                parse_args::Renderer::Accelerated => window.into_canvas().accelerated(),
            }
        }
        .build()
        .unwrap();
        let texture_creator = canvas.texture_creator();
        let (mut agon_texture, mut upscale_texture) =
            make_agon_screen_textures(&texture_creator, (wx, wy), (mode_w, mode_h));

        // clear the screen, so user isn't staring at garbage while things init
        canvas.present();

        let mut last_frame_time = std::time::Instant::now();

        'inner: loop {
            let frame_duration = std::time::Duration::from_micros(1_000_000 / frame_rate_hz as u64);
            let elapsed_since_last_frame = last_frame_time.elapsed();
            if elapsed_since_last_frame < frame_duration {
                std::thread::sleep(std::time::Duration::from_millis(5));
                continue;
            } else if elapsed_since_last_frame > std::time::Duration::from_millis(100) {
                // don't let lots of frames queue up, due to system lag or whatever
                last_frame_time = std::time::Instant::now();
            }

            // Present a frame
            last_frame_time = last_frame_time
                .checked_add(frame_duration)
                .unwrap_or(std::time::Instant::now());

            // signal vsync to ez80 via GPIO (pin 1 (from 0) of GPIO port B)
            {
                // XXX note this is wrong, should be asserted for the whole vblank duration.
                // but we do it here just for an instant since that's sufficient to trigger
                // the interrupt.
                let mut gpios = gpios.lock().unwrap();
                gpios.b.set_input_pin(1, true);
                gpios.b.set_input_pin(1, false);
            }
            // signal vblank to VDP
            unsafe {
                (*vdp_interface.signal_vblank)();
            }

            // shutdown if requested (atomic could be set from the debugger)
            if emulator_shutdown.load(std::sync::atomic::Ordering::Relaxed) {
                break 'running;
            }

            for event in event_pump.poll_iter() {
                match event {
                    Event::Quit { .. } => break 'running,
                    Event::TextInput { ref text, .. } if args.osk => {
                        for b in text.chars() {
                            let vk = ascii2vk::ascii2vk(b);
                            if vk > 0 {
                                unsafe {
                                    (*vdp_interface.sendVKeyEventToFabgl)(vk, 1);
                                    (*vdp_interface.sendVKeyEventToFabgl)(vk, 0);
                                }
                            }
                        }
                    }
                    Event::KeyDown {
                        keycode,
                        scancode,
                        keymod,
                        ..
                    } => {
                        let hostkey = if args.alternative_hostkey {
                            sdl2::keyboard::Mod::RALTMOD
                        } else {
                            sdl2::keyboard::Mod::RCTRLMOD
                        };
                        // handle emulator shortcut keys
                        let consumed = if keymod.contains(hostkey) {
                            match keycode {
                                Some(sdl2::keyboard::Keycode::C) => {
                                    // caps-lock
                                    unsafe {
                                        (*vdp_interface.sendPS2KbEventToFabgl)(0x58, 1);
                                        (*vdp_interface.sendPS2KbEventToFabgl)(0x58, 0);
                                    }
                                    true
                                }
                                Some(sdl2::keyboard::Keycode::F) => {
                                    is_fullscreen = !is_fullscreen;
                                    break 'inner;
                                }
                                Some(sdl2::keyboard::Keycode::M) => {
                                    unsafe {
                                        (*vdp_interface.dump_vdp_mem_stats)();
                                    }
                                    true
                                }
                                Some(sdl2::keyboard::Keycode::Q) => {
                                    break 'running;
                                }
                                Some(sdl2::keyboard::Keycode::R) => {
                                    soft_reset.store(true, std::sync::atomic::Ordering::Relaxed);
                                    true
                                }
                                Some(sdl2::keyboard::Keycode::S) => {
                                    screen_scale = match screen_scale {
                                        parse_args::ScreenScale::StretchAny => {
                                            parse_args::ScreenScale::Scale4_3
                                        }
                                        parse_args::ScreenScale::Scale4_3 => {
                                            parse_args::ScreenScale::ScaleInteger
                                        }
                                        parse_args::ScreenScale::ScaleInteger => {
                                            parse_args::ScreenScale::StretchAny
                                        }
                                    };
                                    true
                                }
                                _ => false,
                            }
                        } else {
                            false
                        };
                        if !consumed {
                            let ps2scancode = sdl2ps2::sdl2ps2(scancode.unwrap());
                            if ps2scancode > 0 {
                                if sdl2ps2::is_not_ascii(scancode.unwrap()) || !args.osk {
                                    unsafe {
                                        (*vdp_interface.sendPS2KbEventToFabgl)(ps2scancode, 1);
                                    }
                                }
                            }
                        }
                    }
                    Event::KeyUp { scancode, .. } => {
                        let ps2scancode = sdl2ps2::sdl2ps2(scancode.unwrap());
                        if ps2scancode > 0 {
                            unsafe {
                                (*vdp_interface.sendPS2KbEventToFabgl)(ps2scancode, 0);
                            }
                        }
                    }
                    Event::MouseButtonUp { mouse_btn, .. } => {
                        mouse_btn_state &= match mouse_btn {
                            sdl2::mouse::MouseButton::Left => !1,
                            sdl2::mouse::MouseButton::Right => !2,
                            sdl2::mouse::MouseButton::Middle => !4,
                            _ => !0,
                        };
                        let packet: [u8; 4] = [8 | mouse_btn_state, 0, 0, 0];
                        unsafe {
                            (*vdp_interface.sendHostMouseEventToFabgl)(&packet[0] as *const u8);
                        }
                    }
                    Event::MouseButtonDown { mouse_btn, .. } => {
                        mouse_btn_state |= match mouse_btn {
                            sdl2::mouse::MouseButton::Left => 1,
                            sdl2::mouse::MouseButton::Right => 2,
                            sdl2::mouse::MouseButton::Middle => 4,
                            _ => 0,
                        };
                        let packet: [u8; 4] = [8 | mouse_btn_state, 0, 0, 0];
                        unsafe {
                            (*vdp_interface.sendHostMouseEventToFabgl)(&packet[0] as *const u8);
                        }
                    }
                    Event::MouseWheel { y, .. } => {
                        let mut packet: [u8; 4] = [8 | mouse_btn_state, 0, 0, 0];
                        packet[3] = y as u8;
                        unsafe {
                            (*vdp_interface.sendHostMouseEventToFabgl)(&packet[0] as *const u8);
                        }
                    }
                    Event::MouseMotion { xrel, yrel, .. } => {
                        let mut packet: [u8; 4] = [8 | mouse_btn_state, 0, 0, 0];
                        if xrel >= 0 {
                            packet[1] = xrel as u8;
                        } else {
                            packet[1] = xrel as u8;
                            packet[0] |= 0x10;
                        }
                        if yrel <= 0 {
                            packet[2] = -yrel as u8;
                        } else {
                            packet[2] = -yrel as u8;
                            packet[0] |= 0x20;
                        }
                        unsafe {
                            (*vdp_interface.sendHostMouseEventToFabgl)(&packet[0] as *const u8);
                        }
                    }
                    Event::JoyHatMotion {
                        which,
                        hat_idx,
                        state,
                        ..
                    } => {
                        joypad::on_hat_motion(&mut gpios.lock().unwrap(), which, hat_idx, state);
                    }
                    Event::JoyButtonUp {
                        which, button_idx, ..
                    } => {
                        joypad::on_button(&mut gpios.lock().unwrap(), which, button_idx, false);
                    }
                    Event::JoyButtonDown {
                        which, button_idx, ..
                    } => {
                        joypad::on_button(&mut gpios.lock().unwrap(), which, button_idx, true);
                    }
                    Event::JoyAxisMotion {
                        which,
                        axis_idx,
                        value,
                        ..
                    } => {
                        joypad::on_axis_motion(&mut gpios.lock().unwrap(), which, axis_idx, value);
                    }
                    Event::JoyDeviceRemoved { .. } => {}
                    Event::JoyDeviceAdded { .. } => {
                        open_joystick_devices(&mut joysticks, &joystick_subsystem);
                    }
                    _ => {}
                }
            }

            {
                let mut w: u32 = 0;
                let mut h: u32 = 0;
                unsafe {
                    (*vdp_interface.copyVgaFramebuffer)(
                        &mut w as *mut u32,
                        &mut h as *mut u32,
                        &mut vgabuf[0] as *mut u8,
                        &mut frame_rate_hz as *mut f32,
                    );
                }

                if w != mode_w || h != mode_h {
                    //println!("Mode change to {} x {}", w, h);
                    mode_w = w;
                    mode_h = h;
                    (agon_texture, upscale_texture) =
                        make_agon_screen_textures(&texture_creator, (wx, wy), (mode_w, mode_h));
                }

                match args.renderer {
                    parse_args::Renderer::Software => {
                        agon_texture.update(None, &vgabuf, 3 * w as usize).unwrap();
                    }
                    parse_args::Renderer::Accelerated => {
                        agon_texture.update(None, &vgabuf, 3 * w as usize).unwrap();
                        /*
                         * This is how it's supposed to be done for a streaming texture,
                         * but it produces a black screen on some systems...
                        agon_texture.with_lock(Some(sdl2::rect::Rect::new(0, 0, w, h)), |data, pitch| {
                            let mut i = 0;
                            for y in 0..h {
                                let row = y as usize * pitch;
                                for x in 0..w {
                                    let col = 3 * x as usize;
                                    data[row + col] = vgabuf[i];
                                    data[row + col + 1] = vgabuf[i+1];
                                    data[row + col + 2] = vgabuf[i+2];
                                    i += 3;
                                }
                             }
                        }).unwrap();
                        */
                    }
                }
            }

            /* Keep rendered output to 4:3 aspect ratio */
            let dst = Some(calc_4_3_output_rect(
                canvas.output_size().unwrap(),
                (mode_w, mode_h),
                screen_scale,
            ));

            canvas.set_draw_color(sdl2::pixels::Color {
                r: (args.border >> 16) as u8,
                g: (args.border >> 8) as u8,
                b: args.border as u8,
                a: 0,
            });
            canvas.clear();

            if screen_scale == parse_args::ScreenScale::ScaleInteger {
                // render directly from agon texture, with no filtering (ScaleModeNearest)
                // to dst that is already calculated as an integer scaling
                canvas.copy(&agon_texture, None, dst).unwrap();
            } else {
                // first perform integer upscale of the agon_texture to upscale_texture
                canvas
                    .with_texture_canvas(&mut upscale_texture, |c| {
                        c.copy(&agon_texture, None, None).unwrap();
                    })
                    .unwrap();
                // then draw upscaled texture to screen (with bilinear/anisotropic filtering)
                // with dst at arbitrary scaling
                canvas.copy(&upscale_texture, None, dst).unwrap();
            }
            canvas.present();
        }
    }

    // signal the shutdown to any other listeners
    emulator_shutdown.store(true, std::sync::atomic::Ordering::Relaxed);

    // give vdp some time to shutdown
    unsafe {
        (*vdp_interface.vdp_shutdown)();
    }
    std::thread::sleep(std::time::Duration::from_millis(200));

    return exit_status.load(std::sync::atomic::Ordering::Relaxed);
}

fn calc_int_scale(canvas_size: (u32, u32), agon_size: (u32, u32)) -> (u32, u32) {
    // icky hack to make 640x240 mode stretch correctly
    let agon_scr_adj = if agon_size == (640, 240) {
        (640, 480)
    } else {
        agon_size
    };
    let int_scale = u32::max(
        1,
        u32::min(
            canvas_size.0 / agon_scr_adj.0,
            canvas_size.1 / agon_scr_adj.1,
        ),
    );
    (int_scale * agon_scr_adj.0, int_scale * agon_scr_adj.1)
}

fn calc_4_3_output_rect(
    window_size: (u32, u32),
    agon_scr: (u32, u32),
    scale: parse_args::ScreenScale,
) -> sdl2::rect::Rect {
    let (wx, wy) = window_size;

    match scale {
        parse_args::ScreenScale::StretchAny => sdl2::rect::Rect::new(0, 0, wx, wy),
        parse_args::ScreenScale::ScaleInteger if wx >= agon_scr.0 && wy >= agon_scr.1 => {
            let scaled_size = calc_int_scale((wx, wy), agon_scr);
            let offx = ((wx - scaled_size.0) / 2) as i32;
            let offy = ((wy - scaled_size.1) / 2) as i32;
            return sdl2::rect::Rect::new(offx, offy, scaled_size.0, scaled_size.1);
        }
        _ => {
            if wx > 4 * wy / 3 {
                sdl2::rect::Rect::new((wx as i32 - 4 * wy as i32 / 3) >> 1, 0, 4 * wy / 3, wy)
            } else {
                sdl2::rect::Rect::new(0, (wy as i32 - 3 * wx as i32 / 4) >> 1, wx, 3 * wx / 4)
            }
        }
    }
}

fn open_joystick_devices(
    joysticks: &mut Vec<sdl2::joystick::Joystick>,
    joystick_subsystem: &sdl2::JoystickSubsystem,
) {
    joysticks.clear();

    for i in 0..joystick_subsystem.num_joysticks().unwrap() {
        joysticks.push(joystick_subsystem.open(i).unwrap());
    }
}

/**
 * Make 2 textures:
 * @return.0 the size of the agon screen (to copy the agon framebuffer to directly)
 * @return.1 and another an integer scaling of this, to provide a clean upscale for subsequent
 *           bilinear-filtered rendering to the SDL screen.
 */
fn make_agon_screen_textures(
    texture_creator: &sdl2::render::TextureCreator<sdl2::video::WindowContext>,
    native_size: (u32, u32),
    agon_size: (u32, u32),
) -> (sdl2::render::Texture, sdl2::render::Texture) {
    let texture = texture_creator
        .create_texture_streaming(
            sdl2::pixels::PixelFormatEnum::RGB24,
            agon_size.0,
            agon_size.1,
        )
        .unwrap();
    let int_scale_size = calc_int_scale(native_size, agon_size);
    let upscale_texture = texture_creator
        .create_texture_target(
            sdl2::pixels::PixelFormatEnum::RGB24,
            int_scale_size.0,
            int_scale_size.1,
        )
        .unwrap();
    // enable filtering of final blit from integer-upscaled agon screen to the SDL screen
    unsafe {
        sdl2_sys::SDL_SetTextureScaleMode(
            upscale_texture.raw(),
            sdl2_sys::SDL_ScaleMode::SDL_ScaleModeBest,
        );
    }

    (texture, upscale_texture)
}
