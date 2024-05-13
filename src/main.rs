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
    generate!("hermes::Buffer")
    safety!(unsafe_ffi)
    generate!("Handle")
    generate!("hermes::vm::GCScopeMarkerRAII")
    //generate!("hermes::vm::Runtime")
    //generate!("hermes::vm::IdentifierTable")
    subclass!("BindingsDefine", RustBindingsDefine)
    generate!("hermes::vm::DefinePropertyFlags")
    generate!("NativeFunctionDefine")
    //generate!("hermes::vm::NativeArgs")
    generate!("Arguments")
    generate!("Value")
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
    generate!("StringPrimitiveHandle")
    generate!("hermes::vm::StringView")
    generate!("executeHBCBytecode")
}

#[subclass]
#[derive(Default)]
pub struct BasicRustJSFunction;

impl ffi::NativeVFunction_methods for BasicRustJSFunction {
    fn invoke(
        &self,
        context: &ffi::BindingsDefine,
        mut runtime: Pin<&mut ffi::hermes::vm::Runtime>,
        args: Pin<&mut ffi::hermes::vm::NativeArgs>
    ) -> cxx::UniquePtr<ffi::NativeVFunctionReturnValue> {
        println!("invoke called with {} args", ffi::Arguments::getArgCount(args.borrow()).0);
        let undefined = ffi::NativeVFunctionReturnValue::encodeUndefined().within_unique_ptr();
        if ffi::Arguments::getArgCount(args.borrow()).0 < 1 {
            return undefined
        }
        {
            let argument_value = ffi::Arguments::getArg(args.borrow(), autocxx::c_uint::from(0)).within_unique_ptr();
            let argument = (|argument_value: &ffi::Value| unsafe {
                match argument_value.isNumber() {
                    true => Some(argument_value.getNumber(std::ptr::null_mut())),
                    false => None,
                }
            })(argument_value.borrow()).expect("expected argument to be a number");
            println!("argment is {}", argument);
        }
        if ffi::Arguments::getArgCount(args.borrow()).0 < 2 {
            return undefined;
        }
        let argument_handle = ffi::Arguments::getArgHandle(args.borrow(), autocxx::c_uint::from(1)).within_unique_ptr();
        let mut argument_string = (|argument_handle: &ffi::Handle| unsafe {
            match argument_handle.isAStringPrimitiveHandle() {
                true => Some(argument_handle.castToStringPrimitiveHandle(std::ptr::null_mut()).within_unique_ptr()),
                false => None,
            }
        })(argument_handle.borrow()).expect("expected argument to be a string");
        let argument_string_view = argument_string.as_mut().expect("invalid pointer to string").createStringView(runtime.as_mut()).within_unique_ptr();
        let argument_encoding =  argument_string_view.as_ref().expect("invalid string view pointer").isASCII();
        println!("argment is in {} encoding", if argument_encoding { "ASCII" } else { "UTF16" });
        if argument_encoding == false {
            unsafe {
                let argument_string_slice = core::slice::from_raw_parts(argument_string_view.castToChar16Ptr(), argument_string_view.length());
                for argument_char16 in argument_string_slice.iter() {
                    print!("{:X?} ", argument_char16.0);
                }
            }
        }
        return undefined;
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
        let symbol_ref = bindings_def.createASCIIRef(function_name_str.as_ref().expect("failed to create ASCII ref"));
        let symbol_handle_result = bindings_def.getSymbolHandleASCII(runtime.as_mut(), symbol_ref).within_unique_ptr();
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
    // buffer should be safe since its lifetime relies on binary outliving it
    // and buffer dies after exit and binary dies after exit
    let buffer = (|binary: &Vec<u8>| unsafe {
        return ffi::hermes::Buffer::new1(binary.as_ptr(), binary.len()).within_unique_ptr();
    })(binary.borrow());
    assert!(!buffer.is_null(), "buffer for hbc bytes was a null pointer");
    let bindings: Rc<RefCell<RustBindingsDefine>> = RustBindingsDefine::new_rust_owned(make_rust_bindings_define());
    bindings.as_ref().borrow_mut().deref_mut().start();
    let success: bool = ffi::executeHBCBytecode(
        buffer,
        input_file_path,
        bindings.as_ref().borrow().as_ref(),
    );
    std::process::exit(if success {0} else {1});
}
