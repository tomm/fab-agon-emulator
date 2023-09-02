use agon_cpu_emulator::{ AgonMachine, AgonMachineConfig, RamInit };
use sdl2::event::Event;
use std::sync::mpsc;
use std::sync::mpsc::{Sender, Receiver};
use std::thread;
use std::time::Duration;

extern "C" {
    fn vdp_setup();
    fn vdp_loop();
    fn copyVgaFramebuffer(outWidth: *mut i32, outHeight: *mut i32, buffer: *mut u8);
    fn z80_send_to_vdp(b: u8);
    fn z80_recv_from_vdp(out: *mut u8) -> bool;
    fn sendHostKbEventToFabgl(ascii: u8, isDown: bool);
}

pub fn main() {
    let (tx_vdp_to_ez80, rx_vdp_to_ez80): (Sender<u8>, Receiver<u8>) = mpsc::channel();
    let (tx_ez80_to_vdp, rx_ez80_to_vdp): (Sender<u8>, Receiver<u8>) = mpsc::channel();

    let vsync_counter_vdp = std::sync::Arc::new(std::sync::atomic::AtomicU32::new(0));
    let vsync_counter_ez80 = vsync_counter_vdp.clone();

    let debugger_con = None;/*if args.debugger {
        let _debugger_thread = thread::spawn(move || {
            agon_light_emulator_debugger::start(tx_cmd_debugger, rx_resp_debugger);
        });
        Some(DebuggerConnection { tx: tx_resp_debugger, rx: rx_cmd_debugger })
    } else {
        None
    };*/

    let _cpu_thread = thread::spawn(move || {
        // Prepare the device
        let mut machine = AgonMachine::new(AgonMachineConfig {
            ram_init: RamInit::Random,
            to_vdp: tx_ez80_to_vdp,
            from_vdp: rx_vdp_to_ez80,
            vsync_counter: vsync_counter_ez80,
            clockspeed_hz: 18_432_000
        });
        machine.set_sdcard_directory(std::path::PathBuf::from("sdcard"));
        machine.start(debugger_con);
        println!("Cpu thread finished.");
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

    let window = video_subsystem.window("FAB Agon Emulator", 1024, 768)
        .position_centered()
        .build()
        .unwrap();

    let mut canvas = window.into_canvas().build().unwrap();
    let texture_creator = canvas.texture_creator();
    let mut texture = texture_creator.create_texture_streaming(sdl2::pixels::PixelFormatEnum::RGB24, 512, 384).unwrap();

    let mut event_pump = sdl_context.event_pump().unwrap();

    'running: loop {

        for event in event_pump.poll_iter() {
            match event {
                Event::Quit {..} => {
                    break 'running
                },
                Event::KeyDown { keycode, scancode, keymod, .. } => {
                    println!("key down {:?}", keycode);

                    unsafe {
                        sendHostKbEventToFabgl(keycode.unwrap() as u8, true);
                    }
                }
                Event::KeyUp { keycode, scancode, keymod, .. } => {
                    unsafe {
                        sendHostKbEventToFabgl(keycode.unwrap() as u8, false);
                    }
                }
                _ => {}
            }
        }

        texture.with_lock(Some(sdl2::rect::Rect::new(0, 0, 512, 384)), |data, _pitch| {
            let mut w: i32 = 0;
            let mut h: i32 = 0;
            unsafe {
                copyVgaFramebuffer(&mut w as *mut i32, &mut h as *mut i32, &mut data[0] as *mut u8);
            }
        }).unwrap();

        canvas.copy(&texture, None, None).unwrap();
        canvas.present();
        vsync_counter_vdp.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
        std::thread::sleep(Duration::from_millis(1));
    }
}
