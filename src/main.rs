use autocxx::prelude::*;
use autocxx::subclass::*;
use ffi::BindingsDefine_methods;
use core::pin::Pin;
use std::env;
use std::fs;

include_cpp! {
    #include "wrapper.hpp"
    safety!(unsafe_ffi)
    generate!("hermes::Buffer")
    generate!("hermes::vm::GCScopeMarkerRAII")
    //generate!("hermes::vm::Runtime")
    subclass!("BindingsDefine", RustBindingsDefine)
    generate!("NativeFunctionDefine")
    concrete!("hermes::vm::Handle<hermes::vm::NativeFunction>", NativeFunctionHandle)
    //generate!("hermes::vm::NativeFunction")
    //generate!("hermes::vm::JSObject")
    //generate!("hermes::vm::ExecutionStatus")
    //generate!("hermes::vm::Predefined")
    //generate!("hermes::vm::createUTF16Ref")
    generate!("executeHBCBytecode")
}

#[subclass]
#[derive(Default)]
pub struct RustBindingsDefine;

impl BindingsDefine_methods for RustBindingsDefine {
    fn install(&self, runtime: Pin<&mut ffi::hermes::vm::Runtime>) {
        println!("install was called");
    }
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
    let bindings = RustBindingsDefine::default_rust_owned();
    let success = ffi::executeHBCBytecode(
        buffer,
        input_file_path,
        bindings.as_ref().borrow().as_ref()
    );
    std::process::exit(if success {0} else {1});
}
