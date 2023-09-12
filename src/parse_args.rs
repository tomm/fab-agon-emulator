const HELP: &str = "\
The Fabulous Agon Emulator!

USAGE:
  fab-agon-emulator [OPTIONS]

FLAGS:
  -h, --help            Prints help information

OPTIONS:
  --sdcard PATH         Sets the path of the emulated SDCard
  --mos PATH            Use a different MOS.bin firmware
  -d, --debugger        Enable the eZ80 debugger
  -f, --fullscreen      Start in fullscreen mode
  -u, --unlimited-cpu   Don't limit eZ80 CPU frequency
";

#[derive(Debug)]
pub struct AppArgs {
    pub sdcard: Option<String>,
    pub debugger: bool,
    pub unlimited_cpu: bool,
    pub fullscreen: bool,
    pub mos_bin: Option<std::path::PathBuf>,
}

pub fn parse_args() -> Result<AppArgs, pico_args::Error> {
    let mut pargs = pico_args::Arguments::from_env();

    if pargs.contains(["-h", "--help"]) {
        print!("{}", HELP);
        std::process::exit(0);
    }

    let args = AppArgs {
        sdcard: pargs.opt_value_from_str("--sdcard")?,
        debugger: pargs.contains(["-d", "--debugger"]),
        unlimited_cpu: pargs.contains(["-u", "--unlimited_cpu"]),
        fullscreen: pargs.contains(["-f", "--fullscreen"]),
        mos_bin: pargs.opt_value_from_str("--mos")?
    };

    let remaining = pargs.finish();
    if !remaining.is_empty() {
        eprintln!("Warning: unused arguments left: {:?}.", remaining);
    }

    Ok(args)
}
