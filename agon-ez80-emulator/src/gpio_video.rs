const GPIO_VGA_FRAME_MAX_HEIGHT: u32 = 1024;
const GPIO_VGA_FRAME_MAX_WIDTH: usize = 1280; // also number of CPU cycles

pub struct GpioVga {
    pub last_vblank_cycle: u64,
    pub last_hblank_cycle: u64,
    pub num_scanlines: u32,
    pub cycles_hblank_to_picture: u64,
    pub scanlines_vblank_to_picture: u32,
    pub last_img_write_cycle: u64,
    pub tx_gpio_vga_frame: std::sync::mpsc::Sender<GpioVgaFrame>,
    pub img: Option<GpioVgaFrame>,
}

pub struct GpioVgaFrame {
    // Note: cycles == pixels
    pub cycles_hblank_to_picture: u64,
    pub line_length_cycles: u64, /* including non-picture parts */
    pub width: u32,
    pub height: u32,
    pub picture: Vec<u8>, // sized GPIO_VGA_FRAME_MAX_WIDTH*GPIO_VGA_FRAME_MAX_HEIGHT
}

/*
 * Strict scanout expects pixels to be scannout out at precise
 * clock cycle timings relative to the vblank assert. This is
 * an extreme test of emulator cycle accuracy (which is not satisfied
 * currently)
 * Lenient scanout just pins the pixels relative to the last hsync
 * assert, which should be much easier for the emulator since the
 * precise timing of PRT interrupts that started the hsync are not
 * needed.
 */
const STRICT_SCANOUT: bool = true;

impl GpioVga {
    pub fn new(tx_gpio_vga_frame: std::sync::mpsc::Sender<GpioVgaFrame>) -> Self {
        GpioVga {
            last_vblank_cycle: 0,
            last_hblank_cycle: 0,
            last_img_write_cycle: 0,
            num_scanlines: 0,
            cycles_hblank_to_picture: 0,
            scanlines_vblank_to_picture: 0,
            tx_gpio_vga_frame,
            img: None,
        }
    }

    pub fn handle_gpioc_write(&mut self, total_cycles_elapsed: u64, old_val: u8, _new_val: u8) {
        if let Some(img) = &mut self.img {
            if STRICT_SCANOUT {
                let pos = img.cycles_hblank_to_picture
                    + (total_cycles_elapsed - self.last_vblank_cycle).wrapping_sub(
                        self.scanlines_vblank_to_picture as u64 * img.line_length_cycles as u64,
                    );
                let prev_pos = img.cycles_hblank_to_picture
                    + (self.last_img_write_cycle - self.last_vblank_cycle).wrapping_sub(
                        self.scanlines_vblank_to_picture as u64 * img.line_length_cycles as u64,
                    );
                for x in prev_pos..pos {
                    if (x as usize) < img.picture.len() {
                        img.picture[x as usize] = old_val;
                    }
                }
            } else {
                let hpos = total_cycles_elapsed - self.last_hblank_cycle;
                let prev_hpos = self.last_img_write_cycle - self.last_hblank_cycle;
                for x in prev_hpos..hpos {
                    let pos = img.line_length_cycles as usize
                        * (self
                            .num_scanlines
                            .wrapping_sub(self.scanlines_vblank_to_picture))
                            as usize
                        + x as usize;
                    if pos < img.picture.len() {
                        img.picture[pos] = old_val;
                    }
                }
            }
            self.last_img_write_cycle = total_cycles_elapsed;
        }
    }

    pub fn handle_gpiod_write(&mut self, total_cycles_elapsed: u64, old_val: u8, new_val: u8) {
        // detect change in vblank level
        if new_val & 0x40 != old_val & 0x40 {
            //println!("Vsync change {}", self.total_cycles_elapsed);
            // ignore end of pulse (activate on start of pulse)
            if total_cycles_elapsed - self.last_vblank_cycle > 200 * 588 {
                let last_vblank = self.last_vblank_cycle;
                self.last_vblank_cycle = total_cycles_elapsed;
                let frame_duration = total_cycles_elapsed - last_vblank;
                /*println!(
                    "{} cycles between vblanks ({} Hz) ({} lines)",
                    frame_duration,
                    18432000.0 / frame_duration as f32,
                    self.num_scanlines
                );*/
                if self.num_scanlines >= 200 && self.num_scanlines <= 600 {
                    // send frame
                    if let Some(img) = self.img.take() {
                        self.tx_gpio_vga_frame.send(img).unwrap();
                    }
                    // Now we have sync, allocate a GpioVgaFrame for the next frame
                    let scanline_duration_cycles = (frame_duration as u32 / self.num_scanlines)
                        .min(1280)
                        .wrapping_add(1)
                        >> 2
                        << 2;
                    // our reading of the scanline starts with hsync assert. how many
                    // cycles to ignore after this before the image?
                    // On vga 640x480 it's 71 + 35 (hsync + back porch). That's 18% of
                    // the total 588 cycles per scanline (46 / 256 = 18%)
                    let cycles_hblank_to_picture = (46 * scanline_duration_cycles / 256) as u64;
                    self.scanlines_vblank_to_picture = match self.num_scanlines {
                        261..=263 => 17, // 320x240@60hz (15khz)
                        524..=526 => 35, // 640x480@60hz
                        448..=450 => 62, // 640x350@70hz
                        _ => 0,          // unknown mode... just render all scanlines
                    };
                    self.img = Some(GpioVgaFrame {
                        cycles_hblank_to_picture,
                        line_length_cycles: scanline_duration_cycles as u64,
                        width: 640 * scanline_duration_cycles / 800,
                        height: match self.num_scanlines {
                            524..=526 => 480,                                       // 640x480@60hz
                            448..=450 => 350,                                       // 640x350@70hz
                            _ => self.num_scanlines.min(GPIO_VGA_FRAME_MAX_HEIGHT), // unknown mode... just render all scanlines
                        },
                        picture: vec![
                            0;
                            GPIO_VGA_FRAME_MAX_WIDTH * GPIO_VGA_FRAME_MAX_HEIGHT as usize
                        ],
                    });
                }
                self.num_scanlines = 0;
            }
        }
        // detect change in hblank level
        if old_val & 0x80 != new_val & 0x80 {
            // ignore end of pulse (activate on start of pulse)
            // Expect at least 500 cycles since last transition to
            // detect and ignore the short transition when hsync un-asserts
            if total_cycles_elapsed - self.last_hblank_cycle > 500 {
                let last_hblank = self.last_hblank_cycle;
                self.last_hblank_cycle = total_cycles_elapsed;
                /*
                let hblank_duration = total_cycles_elapsed - last_hblank;
                println!(
                    "{} cycles between hblanks (on line {})",
                    hblank_duration,
                    self.num_scanlines
                );
                */
                self.num_scanlines += 1;
                self.last_img_write_cycle = total_cycles_elapsed;
            }
        }
    }
}
