fn main() -> Result<(), String> {
    let vdp_version = std::env::var("VDP_VERSION").ok().unwrap_or("console8".to_string());

    let vdp_source = match vdp_version.as_str() {
        "console8" => "./userspace-vdp/vdp-console8.cpp",
        "quark103" => "./userspace-vdp/vdp-1.03.cpp",
        _ => {
            return Err(format!("Invalid env var VDP_VERSION: {}. Valid versions are: console8, quark103", vdp_version));
        }
    };

    println!("cargo:rustc-cfg=vdp_{}", vdp_version);

    cc::Build::new()
        .cpp(true)
        .cpp_link_stdlib("stdc++")
        .warnings(false)
        .extra_warnings(false)
        .include("./userspace-vdp")
        .include("./userspace-vdp/dispdrivers/")
        .file(vdp_source)
        .file("./userspace-vdp/canvas.cpp")
        .file("./userspace-vdp/codepages.cpp")
        .file("./userspace-vdp/collisiondetector.cpp")
        .file("./userspace-vdp/displaycontroller.cpp")
        .file("./userspace-vdp/esp32time.cpp")
        .file("./userspace-vdp/fabfonts.cpp")
        .file("./userspace-vdp/fabutils.cpp")
        .file("./userspace-vdp/fake_misc.cpp")
        .file("./userspace-vdp/HardwareSerial.cpp")
        .file("./userspace-vdp/kbdlayouts.cpp")
        .file("./userspace-vdp/keyboard.cpp")
        .file("./userspace-vdp/ps2controller.cpp")
        .file("./userspace-vdp/soundgen.cpp")
        .file("./userspace-vdp/terminal.cpp")
        .file("./userspace-vdp/terminfo.cpp")
        .file("./userspace-vdp/rust_glue.cpp")
        .file("./userspace-vdp/vga16controller.cpp")
        .file("./userspace-vdp/vga2controller.cpp")
        .file("./userspace-vdp/vga4controller.cpp")
        .file("./userspace-vdp/vga8controller.cpp")
        .file("./userspace-vdp/vgabasecontroller.cpp")
        .file("./userspace-vdp/vgacontroller.cpp")
        .file("./userspace-vdp/vgapalettedcontroller.cpp")
        .compile("hostvdp");

    Ok(())
}
