use autocxx::prelude::*;
use std::env;
use std::fs;

include_cpp! {
    #include "wrapper.hpp"
    safety!(unsafe_ffi)
    generate!("hermes::Buffer")
    generate!("executeHBCBytecode")
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 2 {
        eprintln!("Usage: {} <input_filename>", args[0]);
        std::process::exit(1);
    }
    let input_file_path = &args[1];

    let binary = fs::read(input_file_path)
        .expect("file read fail");
    let buffer: cxx::UniquePtr<ffi::hermes::Buffer> = (|binary: &Vec<u8>| unsafe {
        return ffi::hermes::Buffer::new1(binary.as_ptr(), binary.len()).within_unique_ptr();
    })(binary.as_ref());
    let success = ffi::executeHBCBytecode(
        buffer,
        input_file_path);
    std::process::exit(if success {0} else {1});
}
