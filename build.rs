use std::process::Command;

fn main() {
    Command::new("make").status().expect("failed to build VDP");
}
