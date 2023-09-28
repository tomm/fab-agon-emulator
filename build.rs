use std::process::Command;

fn main() {
    Command::new("git").arg("submodule").arg("update").arg("--init").status().expect("failed to update git submodules");
    Command::new("make").status().expect("failed to build VDP");
}
