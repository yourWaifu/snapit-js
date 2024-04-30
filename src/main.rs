use autocxx::prelude::*;
use autocxx::subclass::*;
use ffi::BindingsDefine_methods;
use core::pin::Pin;
use core::pin::pin;
use std::borrow::Borrow;
use std::borrow::BorrowMut;
use std::ptr::null;
use std::rc::Rc;
use core::cell::RefCell;
use std::env;
use std::fs;
use std::ops::DerefMut;

include_cpp! {
    #include "wrapper.hpp"
    safety!(unsafe_ffi)
    generate!("hermes::Buffer")
    generate!("hermes::vm::GCScopeMarkerRAII")
    //generate!("hermes::vm::Runtime")
    //generate!("hermes::vm::IdentifierTable")
    subclass!("BindingsDefine", RustBindingsDefine)
    generate!("hermes::vm::DefinePropertyFlags")
    generate!("NativeFunctionDefine")
    //generate!("hermes::vm::NativeArgs")
    generate!("NativeVFunctionReturnValue")
    generate!("hermes::vm::ASCIIRef")
    generate!("hermes::vm::SymbolID")
    generate!("SymbolIDHandle")
    generate!("SymbolIDHandleResult")
    concrete!("hermes::vm::Handle<hermes::vm::NativeFunction>", NativeFunctionHandle)
    subclass!("NativeVFunction", BasicRustJSFunction)
    //generate!("hermes::vm::NativeFunction")
    //generate!("hermes::vm::JSObject")
    //generate!("hermes::vm::ExecutionStatus")
    //generate!("hermes::vm::Predefined")
    //generate!("hermes::vm::createUTF16Ref")
    generate!("executeHBCBytecode")
}

#[subclass]
#[derive(Default)]
pub struct BasicRustJSFunction;

impl ffi::NativeVFunction_methods for BasicRustJSFunction {
    fn invoke(
        &self,
        context: &ffi::BindingsDefine,
        runtime: Pin<&mut ffi::hermes::vm::Runtime>/*,*/
        //args: &ffi::hermes::vm::NativeArgs
    ) -> cxx::UniquePtr<ffi::NativeVFunctionReturnValue> {
        println!("invoke called");
        return ffi::NativeVFunctionReturnValue::encodeUndefined().within_unique_ptr();
    }
}

#[subclass]
#[derive(Default)]
pub struct RustBindingsDefine {
    basic_function: Rc<RefCell<BasicRustJSFunction>>,
    basic_function_context: Option<Pin<Box<ffi::BindingsDefine_FunctionContext>>>,
}

fn make_rust_bindings_define() -> RustBindingsDefine {
    let basic_function = BasicRustJSFunction::default_rust_owned();
    RustBindingsDefine {
        basic_function: basic_function,
        basic_function_context: Default::default(),
        cpp_peer: Default::default(), // to do: figure out how to make this a argument
    }
}

impl BindingsDefine_methods for RustBindingsDefine {
    fn start(&mut self) {
        println!("start called");
        self.basic_function_context = Some(self.as_ref().makeFunctionContext(self.basic_function.as_ref().borrow().as_ref()).within_box());
    }

    fn install(&self, mut runtime: Pin<&mut ffi::hermes::vm::Runtime>, bindings_def: &ffi::BindingsDefine) {
        use ffi::ToCppString;
        println!("install called");
        
        let normal_def_property_flags = ffi::hermes::vm::DefinePropertyFlags::getNewNonEnumerableFlags();

        let mut function_bind_def: UniquePtr<ffi::NativeFunctionDefine> = bindings_def.functionWithOnlyRuntime(runtime.as_mut());
        let function_name_str = "nativeTest".into_cpp();
        let symbol_handle_result = bindings_def.getSymbolHandleASCII(runtime.as_mut(), bindings_def.createASCIIRef(function_name_str.as_ref().expect("failed to create ASCII ref"))).within_unique_ptr();
        let symbol_handle = bindings_def.ignoreAllocationFailure(runtime.as_mut(), symbol_handle_result).within_unique_ptr();
        function_bind_def.as_mut().expect("null function bind deref").setFunction(
            symbol_handle.get().within_unique_ptr(),
            &self.basic_function_context.as_ref().expect("basic function context is null"),
            autocxx::c_uint::from(0u32));
        let function_bind_result = function_bind_def.as_ref().expect("null function bind deref").define();
        assert!(function_bind_result, "failed to define function bind");
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() != 2 {
        eprintln!("Usage: {} <input_filename>", args[0]);
        std::process::exit(1);
    }
    let input_file_path = &args[1];

    let binary: Box<Vec<u8>> = Box::new(fs::read(input_file_path)
        .expect("file read fail"));
    // to do, figure out how to make this safe.
    let buffer = (|binary: &Vec<u8>| unsafe {
        return ffi::hermes::Buffer::new1(binary.as_ptr(), binary.len()).within_unique_ptr();
    })(binary.borrow());
    let bindings: Rc<RefCell<RustBindingsDefine>> = RustBindingsDefine::new_rust_owned(make_rust_bindings_define());
    bindings.as_ref().borrow_mut().deref_mut().start();
    let success = ffi::executeHBCBytecode(
        buffer,
        input_file_path,
        bindings.as_ref().borrow().as_ref(),
    );
    std::process::exit(if success {0} else {1});
}
