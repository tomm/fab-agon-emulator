const HELP: &str = "\
Agon CLI Emulator

USAGE:
  agon-cli-emulator [OPTIONS]

OPTIONS:
  -h, --help            Prints help information
  --mos PATH            Use a different MOS.bin firmware
  --ram-size <KiB>      Set a non-standard RAM size (default is 512KiB)
  --sdcard-img <file>   Use a raw SDCard image rather than the host filesystem
  --sdcard <path>       Sets the path of the emulated SDCard
  -u, --unlimited-cpu   Don't limit eZ80 CPU frequency
  -z, --zero            Initialize ram with zeroes instead of random values
";

#[derive(Debug)]
pub struct AppArgs {
    //pub debugger: bool,
    pub sdcard: Option<String>,
    pub sdcard_img: Option<String>,
    pub unlimited_cpu: bool,
    pub zero: bool,
    pub mos_bin: Option<std::path::PathBuf>,
    pub ram_size: u32,
}

pub fn parse_args() -> Result<AppArgs, pico_args::Error> {
    let mut pargs = pico_args::Arguments::from_env();

    // for `make install`
    if pargs.contains("--prefix") {
        print!("{}", option_env!("PREFIX").unwrap_or(""));
        std::process::exit(0);
    }

    if pargs.contains(["-h", "--help"]) {
        print!("{}", HELP);
        std::process::exit(0);
    }

    let args = AppArgs {
        //debugger: pargs.contains(["-d", "--debugger"]),
        sdcard: pargs.opt_value_from_str("--sdcard")?,
        sdcard_img: pargs.opt_value_from_str("--sdcard-img")?,
        unlimited_cpu: pargs.contains(["-u", "--unlimited-cpu"]),
        zero: pargs.contains(["-z", "--zero"]),
        mos_bin: pargs.opt_value_from_str("--mos")?,
        ram_size: pargs
            .value_from_str("--ram-size")
            .unwrap_or(512u32)
            .next_power_of_two()
            .min(16384),
    };

    let remaining = pargs.finish();
    if !remaining.is_empty() {
        eprintln!("Warning: unused arguments left: {:?}.", remaining);
    }

    Ok(args)
}
