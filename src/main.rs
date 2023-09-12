use agon_cpu_emulator::{ AgonMachine, AgonMachineConfig, RamInit };
use agon_cpu_emulator::debugger::{ DebugCmd, DebugResp, DebuggerConnection };
use sdl2::event::Event;
use std::sync::mpsc;
use std::sync::mpsc::{Sender, Receiver};
use std::thread;

use crate::parse_args::parse_args;
mod sdl2ps2;
mod parse_args;

extern "C" {
    fn vdp_setup();
    fn vdp_loop();
    fn copyVgaFramebuffer(outWidth: *mut u32, outHeight: *mut u32, buffer: *mut u8);
    fn z80_send_to_vdp(b: u8);
    fn z80_recv_from_vdp(out: *mut u8) -> bool;
    fn sendHostKbEventToFabgl(ps2scancode: u16, isDown: bool);
    fn getAudioSamples(out: *mut u8, length: u32);
    fn vdp_shutdown();
}

const AUDIO_BUFLEN: usize = 2000;

#[cfg(vdp_quark103)]
const MOS_VER: &'static str = "quark103";
#[cfg(vdp_console8)]
const MOS_VER: &'static str = "quark104rc1";

pub fn main() -> Result<(), pico_args::Error> {
    let args = parse_args()?;

    // Set up various comms channels
    let (tx_vdp_to_ez80, rx_vdp_to_ez80): (Sender<u8>, Receiver<u8>) = mpsc::channel();
    let (tx_ez80_to_vdp, rx_ez80_to_vdp): (Sender<u8>, Receiver<u8>) = mpsc::channel();

    let (tx_cmd_debugger, rx_cmd_debugger): (Sender<DebugCmd>, Receiver<DebugCmd>) = mpsc::channel();
    let (tx_resp_debugger, rx_resp_debugger): (Sender<DebugResp>, Receiver<DebugResp>) = mpsc::channel();

    let vsync_counter_vdp = std::sync::Arc::new(std::sync::atomic::AtomicU32::new(0));
    let vsync_counter_ez80 = vsync_counter_vdp.clone();

    let debugger_con = if args.debugger {
        let _debugger_thread = thread::spawn(move || {
            agon_light_emulator_debugger::start(tx_cmd_debugger, rx_resp_debugger);
        });
        Some(DebuggerConnection { tx: tx_resp_debugger, rx: rx_cmd_debugger })
    } else {
        None
    };

    let _cpu_thread = thread::spawn(move || {
        // Prepare the device
        let mut machine = AgonMachine::new(AgonMachineConfig {
            ram_init: RamInit::Random,
            to_vdp: tx_ez80_to_vdp,
            from_vdp: rx_vdp_to_ez80,
            vsync_counter: vsync_counter_ez80,
            clockspeed_hz: if args.unlimited_cpu { 1000_000_000 } else { 18_432_000 },
            mos_bin: if let Some(mos_bin) = args.mos_bin {
                mos_bin
            } else {
                std::path::PathBuf::from(format!("mos_{}.bin", MOS_VER))
            },
        });
        machine.set_sdcard_directory(std::path::PathBuf::from(args.sdcard.unwrap_or("sdcard".to_string())));
        machine.start(debugger_con);
        panic!("ez80 cpu thread terminated");
    });

    let _comms_thread = thread::spawn(move || {
        loop {
            'z80_to_vdp: loop {
                match rx_ez80_to_vdp.try_recv() {
                    Ok(data) => unsafe {
                        z80_send_to_vdp(data);
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
                    if !z80_recv_from_vdp(&mut data as *mut u8) {
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
            vdp_setup();
            vdp_loop();
        }
    });

    let sdl_context = sdl2::init().unwrap();
    let video_subsystem = sdl_context.video().unwrap();
    let audio_subsystem = sdl_context.audio().unwrap();
    let mut event_pump = sdl_context.event_pump().unwrap();

    let desired_spec = sdl2::audio::AudioSpecDesired {
        freq: Some(16384), // real VDP uses 16384Hz
        channels: Some(1),
        samples: None,
    };

    let device: sdl2::audio::AudioQueue<u8> = audio_subsystem.open_queue(None, &desired_spec).unwrap();
    device.resume();

    let mut is_fullscreen = args.fullscreen;
    // large enough for any agon video mode
    let mut vgabuf: [u8; 1024*768*3] = [0; 1024*768*3];
    let mut mode_w = 640;
    let mut mode_h = 480;

    // doesn't seem to work...
    sdl2::hint::set("SDL_HINT_RENDER_SCALE_QUALITY", "2");

    'running: loop {

        let mut window = video_subsystem.window("Fab Agon Emulator", 1024, 768)
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

            // fill audio buffer
            if device.size() < AUDIO_BUFLEN as u32 {
                let mut audio_buf: [u8; AUDIO_BUFLEN] = [0; AUDIO_BUFLEN];
                unsafe {
                    getAudioSamples(&mut audio_buf as *mut u8, AUDIO_BUFLEN as u32);
                };
                device.queue_audio(&audio_buf).unwrap();
            }

            // Present a frame
            last_frame_time = last_frame_time.checked_add(frame_duration).unwrap_or(std::time::Instant::now());

            // signal the vsync to the ez80
            vsync_counter_vdp.fetch_add(1, std::sync::atomic::Ordering::Relaxed);

            for event in event_pump.poll_iter() {
                match event {
                    Event::Quit {..} => {
                        break 'running
                    },
                    Event::KeyDown { keycode, scancode, keymod, .. } => {
                        // handle emulator shortcut keys
                        if keymod.contains(sdl2::keyboard::Mod::RALTMOD) {
                            match keycode {
                                Some(sdl2::keyboard::Keycode::F) => {
                                    is_fullscreen = !is_fullscreen;
                                    break 'inner;
                                }
                                Some(sdl2::keyboard::Keycode::Q) => {
                                    break 'running;
                                }
                                _ => {}
                            }
                        }
                        let ps2scancode = sdl2ps2::sdl2ps2(scancode.unwrap());
                        if ps2scancode > 0 {
                            unsafe {
                                sendHostKbEventToFabgl(ps2scancode, true);
                            }
                        }
                    }
                    Event::KeyUp { scancode, .. } => {
                        let ps2scancode = sdl2ps2::sdl2ps2(scancode.unwrap());
                        if ps2scancode > 0 {
                            unsafe {
                                sendHostKbEventToFabgl(ps2scancode, false);
                            }
                        }
                    }
                    _ => {}
                }
            }

            {
                let mut w: u32 = 0;
                let mut h: u32 = 0;
                unsafe {
                    copyVgaFramebuffer(&mut w as *mut u32, &mut h as *mut u32, &mut vgabuf[0] as *mut u8);
                }

                if w != mode_w || h != mode_h {
                    println!("Mode change to {} x {}", w, h);
                    mode_w = w;
                    mode_h = h;
                    texture = texture_creator.create_texture_streaming(sdl2::pixels::PixelFormatEnum::RGB24, mode_w, mode_h).unwrap();
                }

                texture.with_lock(Some(sdl2::rect::Rect::new(0, 0, w, h)), |data, _pitch| {
                    for i in 0..w*h*3 {
                        data[i as usize] = vgabuf[i as usize];
                    }
                }).unwrap();
            }

            canvas.copy(&texture, None, None).unwrap();
            canvas.present();
        }
    }
    println!("Shutting down fabgl+vdp...");
    unsafe { vdp_shutdown(); }
    std::thread::sleep(std::time::Duration::from_millis(200));

    Ok(())
}
