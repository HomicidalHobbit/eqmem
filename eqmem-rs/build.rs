use std::env;
use std::path::Path;

fn main() {
    let dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let lib_path = set_libs();
    println!(
        "cargo:rustc-link-search=native={}",
        Path::new(&dir).join(lib_path).display()
    );
}

#[cfg(target_os = "macos")]
fn set_libs() -> String {
    println!("cargo:rustc-link-lib=dylib=c++");
    println!("cargo:rustc-link-lib=dylib=iconv");
    String::from("../lib/macos")
}
#[cfg(target_os = "linux")]
fn set_libs() -> String {
    println!("cargo:rustc-link-lib=static=stdc++");
    String::from("../lib/linux")
}
#[cfg(target_os = "windows")]
fn set_libs() -> String {
    String::from("../lib/win64")
}
