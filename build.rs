fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();

    if target_os == "windows" {
        // Download https://github.com/libsdl-org/SDL/releases/download/release-2.28.3/SDL2-devel-2.28.3-mingw.tar.gz,
        // extract, rename directory from SDL2-2.28.3 to SDL2, and you are ready
        println!("cargo:rustc-link-search=./SDL2/x86_64-w64-mingw32/lib");
    }

    if target_os != "windows" && target_os != "macos" {
        system_deps::Config::new().probe().unwrap();
    }
    if std::env::var("FORCE").is_err() {
        eprintln!(
            "Don't invoke `cargo build` directly. Try using 'make' to build fab-agon-emulator."
        );
        eprintln!("If you really know what you're doing, try again with `FORCE=1 cargo build`");
        std::process::exit(0);
    }
}
