use sdl3::audio::{AudioCallback, AudioStream};

#[allow(non_snake_case)]
pub struct VdpAudioStream {
    pub buffer: Vec<u8>,
    pub getAudioSamples:
        libloading::Symbol<'static, unsafe extern "C" fn(out: *mut u8, length: u32)>,
}
impl AudioCallback<u8> for VdpAudioStream {
    fn callback(&mut self, stream: &mut AudioStream, requested: i32) {
        self.buffer.resize(requested as usize, 0);

        unsafe {
            (*self.getAudioSamples)(&mut self.buffer[0] as *mut u8, requested as u32);
        };

        match stream.put_data(&self.buffer) {
            Ok(()) => {}
            Err(err) => println!("Failed to put audio data: {err}"),
        }
    }
}
