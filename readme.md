## Snapit JS

Fast and memory efficient JavaScript runtime for front-ends by using ahead of time compiling.

## Build
Note: only building on Windows is supported

1. [Download hermes version rn/0.74-stable (github.com)](https://github.com/facebook/hermes/tree/rn/0.74-stable)
2. Build it but don't follow their build instructions because they are incorrect
  Watch the build output, you'll want to know where the library files are outputted for later.
  Look for text like "Build files have been written to:" and "Linking CXX static library".

Afterwards, find the following library files in the build folder, and copy all of them to ``snapit-js/external/``

library |   Windows
-----   |   -----
hermesVMRuntime |   lib\VM\hermesVMRuntime.lib
hermesAST   |   lib\AST\hermesAST.lib
hermesHBCBackend    |   lib\BCGen\HBC\hermesHBCBackend.lib
hermesBackend   |   lib\BCGen\hermesBackend.lib
hermesOptimizer |   lib\hermesOptimizer.lib
hermesFrontend  |   lib\hermesFrontend.lib
hermesParser    |   lib\Parser\hermesParser.lib
hermesSupport   |   lib\Support\hermesSupport.lib
dtoa    |   external\dtoa\dtoa.lib
hermesSourceMap |   lib\SourceMap\hermesSourceMap.lib
hermesADT   |   lib\ADT\hermesADT.lib
hermesInst  |   lib\Inst\hermesInst.lib
hermesFrontEndDefs  |   lib\FrontEndDefs\hermesFrontEndDefs.lib
hermesRegex |   lib\Regex\hermesRegex.lib
hermesPlatform  |   lib\Platform\hermesPlatform.lib
hermesInternalBytecode  |   lib\InternalBytecode\hermesInternalBytecode.lib
hermesPublic    |   public\hermes\Public\hermesPublic.lib
hermesInstrumentation   |   lib\VM\Instrumentation\hermesInstrumentation.lib
hermesPlatformUnicode   |   lib\Platform\Unicode\hermesPlatformUnicode.lib
LLVHSupport |   external\llvh\lib\Support\LLVHSupport.lib
LLVHDemangle    |    external\llvh\lib\Demangle\LLVHDemangle.lib
hermesc    |    bin\hermesc .exe and .pdb

find the following directories and copy them to ``snapit-js/external/``

All OSs |   Windows |
----    |   ----
{hermes build output}\external\llvh\include\  ->  include |   
include\  ->  include |  
external\llvh\include\  ->  include |    
external\llvh\gen\include\  ->  include |    
public\hermes\  ->  include\hermes |    
external\flowparser\include\  ->  include |    
external\icu_decls\  -> include  |


change current directory to snapit-js

```shell
cd snapit-js
```

run cargo build
```
cargo build
```

run example
```
cargo run test.hbc
```

## Use

build JS code

```powershell
.\external\hermesc .\path\to\code.js -emit-binary -out path\to\output.hbc
```

run JS code

```
cargo run path\to\output.hbc
```

## Performance trade-offs

Since all code that runs needs to go to the compiler first, all eval or dynamic imports will need to be known ahead of time or otherwise have to wait for the compiler before the module can run.