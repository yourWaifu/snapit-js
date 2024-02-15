use std::path::PathBuf;

fn main() -> miette::Result<()>  {
    println!("cargo:rustc-link-search=./external");
    println!("cargo:rustc-link-lib=hermesVMRuntime");

    let mut b = autocxx_build::Builder::new("src/main.rs",
        &[
            &PathBuf::from("src"),
            &PathBuf::from("external/include"),
        ]
    )
    .extra_clang_args(&[
        "-std=c++17",
        "-D HERMESVM_HEAP_SEGMENT_SIZE_KB=4096",
        "-D HERMESVM_GC_HADES",
        "-D HERMESVM_ALLOW_COMPRESSED_POINTERS",
        "-D HERMESVM_ALLOW_CONCURRENT_GC",
        "-D HERMESVM_ALLOW_CONTIGUOUS_HEAP",
        "-D HERMESVM_ALLOW_INLINE_ASM",
        "-D HERMES_ENABLE_DEBUGGER",
        "-D HERMES_LLVM",
        "-D HERMES_MEMORY_INSTRUMENTATION",
        "-D HERMES_RELEASE_VERSION=\"0.12.0\"",
        "-D _WINDOWS",
        "-D WIN32",
        "-D _CRT_USE_BUILTIN_OFFSETOF",
        "-DNDEBUG",
    ])
    .build()?;
    b.flag_if_supported("-std=c++17")
        .flag_if_supported("-DHERMESVM_HEAP_SEGMENT_SIZE_KB=4096")
        .flag_if_supported("-DHERMESVM_GC_HADES")
        .flag_if_supported("-DHERMESVM_ALLOW_COMPRESSED_POINTERS")
        .flag_if_supported("-DHERMESVM_ALLOW_CONCURRENT_GC")
        .flag_if_supported("-DHERMESVM_ALLOW_CONTIGUOUS_HEAP")
        .flag_if_supported("-DHERMESVM_ALLOW_INLINE_ASM")
        .flag_if_supported("-DHERMES_ENABLE_DEBUGGER")
        .flag_if_supported("-DHERMES_LLVM")
        .flag_if_supported("-DHERMES_MEMORY_INSTRUMENTATION")
        .flag_if_supported("-DHERMES_RELEASE_VERSION=\"0.12.0\"")
        .flag_if_supported("-D_WINDOWS")
        .flag_if_supported("-DWIN32")
        .flag_if_supported("-D_CRT_USE_BUILTIN_OFFSETOF")
        .flag_if_supported("/std:c++17")
        .flag_if_supported("/DNDEBUG")
        .file("src/wrapper.cpp")
        .compile("snapitjs");

    println!("cargo:rustc-link-lib=hermesAST");
    println!("cargo:rustc-link-lib=hermesHBCBackend");
    println!("cargo:rustc-link-lib=hermesBackend");
    println!("cargo:rustc-link-lib=hermesOptimizer");
    println!("cargo:rustc-link-lib=hermesFrontend");
    println!("cargo:rustc-link-lib=hermesParser");
    println!("cargo:rustc-link-lib=hermesSupport");
    println!("cargo:rustc-link-lib=dtoa");
    println!("cargo:rustc-link-lib=hermesSourceMap");
    println!("cargo:rustc-link-lib=hermesADT");
    println!("cargo:rustc-link-lib=hermesInst");
    println!("cargo:rustc-link-lib=hermesFrontEndDefs");
    println!("cargo:rustc-link-lib=hermesRegex");
    println!("cargo:rustc-link-lib=hermesPlatform");
    println!("cargo:rustc-link-lib=hermesInternalBytecode");
    println!("cargo:rustc-link-lib=hermesPublic");
    println!("cargo:rustc-link-lib=hermesInstrumentation");
    println!("cargo:rustc-link-lib=hermesPlatformUnicode");
    println!("cargo:rustc-link-lib=LLVHSupport");
    println!("cargo:rustc-link-lib=LLVHDemangle");

    println!("cargo:rustc-link-lib=icuuc");
    println!("cargo:rustc-link-lib=icuin");
    println!("cargo:rustc-link-lib=user32");
    println!("cargo:rustc-link-lib=gdi32");
    println!("cargo:rustc-link-lib=winspool");
    println!("cargo:rustc-link-lib=shell32");
    println!("cargo:rustc-link-lib=ole32");
    println!("cargo:rustc-link-lib=oleaut32");
    println!("cargo:rustc-link-lib=uuid");
    println!("cargo:rustc-link-lib=comdlg32");
    println!("cargo:rustc-link-lib=advapi32");

    println!("cargo:rerun-if-changed=src/main.rs");
    println!("cargo:rerun-if-changed=src/wrapper.hpp");
    println!("cargo:rerun-if-changed=src/wrapper.cpp");
    Ok(())
}
