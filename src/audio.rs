use sdl2::audio::AudioCallback;

#[allow(non_snake_case)]
pub struct VdpAudioStream {
    pub getAudioSamples: libloading::Symbol<'static, unsafe extern fn(out: *mut u8, length: u32)>,
}

impl AudioCallback for VdpAudioStream {
    type Channel = u8;

    fn callback(&mut self, out: &mut[u8]) {
        unsafe {
            (*self.getAudioSamples)(&mut out[0] as *mut u8, out.len() as u32);
        }
    }
}
