use agon_cpu_emulator::{ AgonMachine, AgonMachineConfig, RamInit, gpio };
use agon_cpu_emulator::debugger::{ DebugCmd, DebugResp, DebuggerConnection, Trigger };
use sdl2::event::Event;
use std::sync::mpsc;
use std::sync::mpsc::{Sender, Receiver};
use std::sync::{ Arc, Mutex };
use std::thread;
use crate::parse_args::parse_args;
mod sdl2ps2;
mod parse_args;
mod vdp_interface;
mod audio;
mod joypad;

const AUDIO_BUFLEN: u16 = 256;

pub fn main() -> Result<(), pico_args::Error> {
    let args = parse_args()?;
    let vdp_interface = vdp_interface::init(&format!("firmware/vdp_{:?}.so", args.firmware), &args);

    // Set up various comms channels
    let (tx_vdp_to_ez80, rx_vdp_to_ez80): (Sender<u8>, Receiver<u8>) = mpsc::channel();
    let (tx_ez80_to_vdp, rx_ez80_to_vdp): (Sender<u8>, Receiver<u8>) = mpsc::channel();

    let (tx_cmd_debugger, rx_cmd_debugger): (Sender<DebugCmd>, Receiver<DebugCmd>) = mpsc::channel();
    let (tx_resp_debugger, rx_resp_debugger): (Sender<DebugResp>, Receiver<DebugResp>) = mpsc::channel();

    let gpios = Arc::new(Mutex::new(gpio::GpioSet::new()));

    if let Some(breakpoint) = args.breakpoint {
        if let Ok(breakpoint) = u32::from_str_radix(&breakpoint, 16) {
            let trigger = Trigger{
                    address: breakpoint,
                    once: false,
                    actions: vec![DebugCmd::Pause, DebugCmd::Message("CPU paused at initial breakpoint".to_owned()), DebugCmd::GetState],
                };
            let debug_cmd = DebugCmd::AddTrigger(trigger);
            _  = tx_cmd_debugger.send(debug_cmd);
            _  = tx_cmd_debugger.send(DebugCmd::Continue);
        } else {
            println!("Cannot parse breakpoint as hexadecimal. Ignoring.")
        }
    }

    // Atomics for various state communication
    let emulator_shutdown = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    let soft_reset = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));

    // Preserve stdin state, as debugger can leave stdin in raw mode
    #[cfg(target_os = "linux")]
    let _tty = raw_tty::TtyWithGuard::new(std::io::stdin()).unwrap();

    let debugger_con = if args.debugger {
        let _emulator_shutdown = emulator_shutdown.clone();
        let _debugger_thread = thread::spawn(move || {
            agon_light_emulator_debugger::start(tx_cmd_debugger, rx_resp_debugger, _emulator_shutdown);
        });
        Some(DebuggerConnection { tx: tx_resp_debugger, rx: rx_cmd_debugger})
    } else {
        None
    };

    let _cpu_thread = {
        let soft_reset_ez80 = soft_reset.clone();
        let gpios_ = gpios.clone();

        thread::spawn(move || {
            // Prepare the device
            let mut machine = AgonMachine::new(AgonMachineConfig {
                ram_init: if args.zero { RamInit::Zero} else {RamInit::Random},
                to_vdp: tx_ez80_to_vdp,
                from_vdp: rx_vdp_to_ez80,
                gpios: gpios_,
                soft_reset: soft_reset_ez80,
                clockspeed_hz: if args.unlimited_cpu { 1000_000_000 } else { 18_432_000 },
                mos_bin: if let Some(mos_bin) = args.mos_bin {
                    mos_bin
                } else {
                    std::path::PathBuf::from(format!("firmware/mos_{:?}.bin", args.firmware))
                },
            });
            machine.set_sdcard_directory(std::path::PathBuf::from(args.sdcard.unwrap_or("sdcard".to_string())));
            machine.start(debugger_con);
            panic!("ez80 cpu thread terminated");
        })
    };

    let _comms_thread = thread::spawn(move || {
        loop {
            'z80_to_vdp: loop {
                match rx_ez80_to_vdp.try_recv() {
                    Ok(data) => unsafe {
                        (*vdp_interface.z80_send_to_vdp)(data);
                    }
                    Err(mpsc::TryRecvError::Disconnected) => panic!(),
                    Err(mpsc::TryRecvError::Empty) => {
                        break 'z80_to_vdp;
                    }
                }
            }

            'vdp_to_z80: loop {
                let mut data: u8 = 0;
                unsafe {
                    if !(*vdp_interface.z80_recv_from_vdp)(&mut data as *mut u8) {
                        break 'vdp_to_z80;
                    }
                    tx_vdp_to_ez80.send(data).unwrap();
                }
            }

            std::thread::sleep(std::time::Duration::from_micros(100));
        }
    });

    // VDP thread
    let _vdp_thread = thread::spawn(move || {
        unsafe {
            (*vdp_interface.vdp_setup)();
            (*vdp_interface.vdp_loop)();
        }
    });

    let sdl_context = sdl2::init().unwrap();
    let native_resolution = sdl_context.video().unwrap().current_display_mode(0).unwrap();
    let video_subsystem = sdl_context.video().unwrap();
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
                        getAudioSamples: vdp_interface.getAudioSamples
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

    let mut is_fullscreen = args.fullscreen;
    // large enough for any agon video mode
    let mut vgabuf: Vec<u8> = Vec::with_capacity(1024*768*3);
    unsafe { vgabuf.set_len(1024*768*3); }
    let mut mode_w: u32 = 640;
    let mut mode_h: u32 = 480;
    let mut mouse_btn_state: u8 = 0;

    joypad::clear_state(&mut gpios.lock().unwrap());

    'running: loop {
        let (wx, wy): (u32, u32) = {
            if is_fullscreen {
                (native_resolution.w as u32, native_resolution.h as u32)
            } else if let Some(max_height) = args.perfect_scale {
                let scale = u32::max(1, max_height as u32 / mode_h);
                // deal with weird aspect ratios
                match 10*mode_w / mode_h {
                    // 2.66 (640x240)
                    26 =>
                        // scale & !1 keeps the scale divisible by 2, needed for the width scaling
                        ((mode_w * (scale & !1)) >> 1, mode_h * (scale & !1)),
                    // 1.6 (320x200)
                    16 => (10 * mode_w * scale / 12, mode_h * scale),
                    // 1.33
                    13 | _ => (mode_w * scale, mode_h * scale)
                }
            } else {
                // only reached on startup
                (640, 480)
            }
        };

        sdl_context.mouse().set_relative_mouse_mode(is_fullscreen);

        let mut window = video_subsystem.window(&format!("Fab Agon Emulator {}", env!("CARGO_PKG_VERSION")), wx, wy)
            .resizable()
            .position_centered()
            .build()
            .unwrap();

        if is_fullscreen {
            window.set_fullscreen(sdl2::video::FullscreenType::True).unwrap();
        }

        let mut canvas = window.into_canvas().build().unwrap();
        let texture_creator = canvas.texture_creator();
        let mut texture = texture_creator.create_texture_streaming(sdl2::pixels::PixelFormatEnum::RGB24, mode_w, mode_h).unwrap();

        // clear the screen, so user isn't staring at garbage while things init
        canvas.present();

        let mut last_frame_time = std::time::Instant::now();
        // XXX assumes 60Hz video mode
        let frame_duration = std::time::Duration::from_micros(16666);

        'inner: loop {
            let elapsed_since_last_frame = last_frame_time.elapsed();
            if elapsed_since_last_frame < frame_duration {
                std::thread::sleep(std::time::Duration::from_millis(1));
                continue;
            }
            else if elapsed_since_last_frame > std::time::Duration::from_millis(100) {
                // don't let lots of frames queue up, due to system lag or whatever
                last_frame_time = std::time::Instant::now();
            }

            // Present a frame
            last_frame_time = last_frame_time.checked_add(frame_duration).unwrap_or(std::time::Instant::now());

            // signal vsync to ez80 via GPIO (pin 1 (from 0) of GPIO port B)
            {
                let mut gpios = gpios.lock().unwrap();
                gpios.b.set_input_pin(1, true);
                gpios.b.set_input_pin(1, false);
            }

            // shutdown if requested (atomic could be set from the debugger)
            if emulator_shutdown.load(std::sync::atomic::Ordering::Relaxed) {
                break 'running;
            }

            for event in event_pump.poll_iter() {
                match event {
                    Event::Quit {..} => {
                        break 'running
                    },
                    Event::KeyDown { keycode, scancode, keymod, .. } => {
                        // handle emulator shortcut keys
                        let consumed = if keymod.contains(sdl2::keyboard::Mod::RALTMOD) {
                            match keycode {
                                Some(sdl2::keyboard::Keycode::C) => {
                                    // caps-lock
                                    unsafe {
                                        (*vdp_interface.sendHostKbEventToFabgl)(0x58, 1);
                                        (*vdp_interface.sendHostKbEventToFabgl)(0x58, 0);
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
                                _ => false
                            }
                        } else {
                            false
                        };
                        if !consumed {
                            let ps2scancode = sdl2ps2::sdl2ps2(scancode.unwrap());
                            if ps2scancode > 0 {
                                unsafe {
                                    (*vdp_interface.sendHostKbEventToFabgl)(ps2scancode, 1);
                                }
                            }
                        }
                    }
                    Event::KeyUp { scancode, .. } => {
                        let ps2scancode = sdl2ps2::sdl2ps2(scancode.unwrap());
                        if ps2scancode > 0 {
                            unsafe {
                                (*vdp_interface.sendHostKbEventToFabgl)(ps2scancode, 0);
                            }
                        }
                    }
                    Event::MouseButtonUp { mouse_btn, .. } => {
                        mouse_btn_state &= match mouse_btn {
                            sdl2::mouse::MouseButton::Left => !1,
                            sdl2::mouse::MouseButton::Right => !2,
                            sdl2::mouse::MouseButton::Middle => !4,
                            _ => !0
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
                            _ => 0
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
                    Event::JoyHatMotion { which, hat_idx, state, ..} => {
                        joypad::on_hat_motion(&mut gpios.lock().unwrap(), which, hat_idx, state);
                    }
                    Event::JoyButtonUp { which, button_idx, .. } => {
                        joypad::on_button(&mut gpios.lock().unwrap(), which, button_idx, false);
                    }
                    Event::JoyButtonDown { which, button_idx, .. } => {
                        joypad::on_button(&mut gpios.lock().unwrap(), which, button_idx, true);
                    }
                    Event::JoyAxisMotion { which, axis_idx, value, .. } => {
                        joypad::on_axis_motion(&mut gpios.lock().unwrap(), which, axis_idx, value);
                    }
                    Event::JoyDeviceRemoved { .. } => {
                    }
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
                    (*vdp_interface.copyVgaFramebuffer)(&mut w as *mut u32, &mut h as *mut u32, &mut vgabuf[0] as *mut u8);
                }

                if w != mode_w || h != mode_h {
                    //println!("Mode change to {} x {}", w, h);
                    mode_w = w;
                    mode_h = h;
                    texture = texture_creator.create_texture_streaming(sdl2::pixels::PixelFormatEnum::RGB24, mode_w, mode_h).unwrap();
                    // may need to resize the window
                    if args.perfect_scale.is_some() && !is_fullscreen {
                        break 'inner;
                    }
                }

                texture.with_lock(Some(sdl2::rect::Rect::new(0, 0, w, h)), |data, _pitch| {
                    for i in 0..w*h*3 {
                        data[i as usize] = vgabuf[i as usize];
                    }
                }).unwrap();
            }

            /* Keep rendered output to 4:3 aspect ratio */
            let dst = Some(calc_4_3_output_rect(&canvas));

            canvas.clear();
            canvas.copy(&texture, None, dst).unwrap();
            canvas.present();
        }
    }

    // signal the shutdown to any other listeners
    emulator_shutdown.store(true, std::sync::atomic::Ordering::Relaxed);

    // give vdp some time to shutdown
    unsafe { (*vdp_interface.vdp_shutdown)(); }
    std::thread::sleep(std::time::Duration::from_millis(200));

    Ok(())
}

fn calc_4_3_output_rect<T: sdl2::render::RenderTarget>(canvas: &sdl2::render::Canvas<T>) -> sdl2::rect::Rect {
    let (wx, wy) = canvas.output_size().unwrap();
    if wx > 4*wy/3 {
        sdl2::rect::Rect::new(
            (wx as i32 - 4*wy as i32/3) >> 1,
            0,
            4*wy/3, wy
        )
    } else {
        sdl2::rect::Rect::new(
            0,
            (wy as i32 - 3*wx as i32/4) >> 1,
            wx, 3*wx/4
        )
    }
}

fn open_joystick_devices(joysticks: &mut Vec<sdl2::joystick::Joystick>, joystick_subsystem: &sdl2::JoystickSubsystem) {
    joysticks.clear();

    for i in 0..joystick_subsystem.num_joysticks().unwrap() {
        joysticks.push(joystick_subsystem.open(i).unwrap());
    }
}
