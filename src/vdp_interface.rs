use std::path::Path;

#[allow(non_snake_case)]
pub struct VdpInterface {
    pub vdp_setup: libloading::Symbol<'static, unsafe extern fn() -> ()>,
    pub vdp_loop: libloading::Symbol<'static, unsafe extern fn()>,
    pub copyVgaFramebuffer: libloading::Symbol<'static, unsafe extern fn(outWidth: *mut u32, outHeight: *mut u32, buffer: *mut u8)>,
    pub z80_send_to_vdp: libloading::Symbol<'static, unsafe extern fn(b: u8)>,
    pub z80_recv_from_vdp: libloading::Symbol<'static, unsafe extern fn(out: *mut u8) -> bool>,
    pub sendHostKbEventToFabgl: libloading::Symbol<'static, unsafe extern fn(ps2scancode: u16, isDown: u8)>,
    pub sendHostMouseEventToFabgl: libloading::Symbol<'static, unsafe extern fn(mouse_packet: *const u8)>,
    pub getAudioSamples: libloading::Symbol<'static, unsafe extern fn(out: *mut u8, length: u32)>,
    pub dump_vdp_mem_stats: libloading::Symbol<'static, unsafe extern fn()>,
    pub vdp_shutdown: libloading::Symbol<'static, unsafe extern fn()>,
}

impl VdpInterface {
    fn new(lib: &'static libloading::Library) -> Self {
        unsafe {
            return VdpInterface {
                vdp_setup: lib.get(b"vdp_setup").unwrap(),
                vdp_loop: lib.get(b"vdp_loop").unwrap(),
                copyVgaFramebuffer: lib.get(b"copyVgaFramebuffer").unwrap(),
                z80_send_to_vdp: lib.get(b"z80_send_to_vdp").unwrap(),
                z80_recv_from_vdp: lib.get(b"z80_recv_from_vdp").unwrap(),
                sendHostKbEventToFabgl: lib.get(b"sendHostKbEventToFabgl").unwrap(),
                sendHostMouseEventToFabgl: lib.get(b"sendHostMouseEventToFabgl").unwrap(),
                getAudioSamples: lib.get(b"getAudioSamples").unwrap(),
                dump_vdp_mem_stats: lib.get(b"dump_vdp_mem_stats").unwrap(),
                vdp_shutdown: lib.get(b"vdp_shutdown").unwrap(),
            }
        }
    }
}

pub fn init(default_vdp: &str, args: &crate::parse_args::AppArgs) -> VdpInterface {
    assert!(unsafe { VDP_DLL == std::ptr::null() });

    let vdp_dll_path = match args.vdp_dll {
        Some(ref p) => Path::new(".").join(p),
        None => Path::new(".").join(default_vdp)
    };

    unsafe {
        VDP_DLL = Box::leak(Box::new(libloading::Library::new(vdp_dll_path).unwrap()));
    }
    VdpInterface::new(unsafe { VDP_DLL.as_ref() }.unwrap())
}
static mut VDP_DLL: *const libloading::Library = std::ptr::null();
