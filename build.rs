fn main() {
    if std::env::var("FORCE").is_err() {
        eprintln!("Don't invoke `cargo build` directly. Try using 'make' to build fab-agon-emulator.");
        eprintln!("If you really know what you're doing, try again with `FORCE=1 cargo build`");
        std::process::exit(1);
    }
}
