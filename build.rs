fn main() {
    let target_os = std::env::var("CARGO_CFG_TARGET_OS").unwrap();

    if target_os == "windows" {
        // Download https://github.com/libsdl-org/SDL/releases/download/release-3.2.28/SDL3-devel-3.2.28-mingw.tar.gz
        // extract, rename directory from SDL3-3.2.28 to SDL3, and you are ready
        println!("cargo:rustc-link-search=./SDL3/x86_64-w64-mingw32/lib");
    }

    if target_os != "windows" && target_os != "macos" {
        //system_deps::Config::new().probe().unwrap();
    }
    if std::env::var("FORCE").is_err() {
        eprintln!(
            "Don't invoke `cargo build` directly. Try using 'make' to build fab-agon-emulator."
        );
        eprintln!("If you really know what you're doing, try again with `FORCE=1 cargo build`");
        std::process::exit(0);
    }
}
