## Snapit JS

Fast and memory efficient JavaScript runtime for front-ends by using ahead of time compiling.

## Build
Note: only building on Windows is supported

[Download the source code and follow the build instructions from HermesJS. (hermesengine.dev)](https://hermesengine.dev/docs/building-and-running/)

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

find the following directories and copy them to ``snapit-js/external/``

All OSs |   Windows |
----    |   ----
{hermes build output}\external\llvh\include\   |   
include\ |  
external\llvh\include\  |    

change current directory to snapit-js

```shell
cd snapit-js
```

run cargo build
```
cargo build
```

## Performance trade-offs

Since all code that runs needs to go to the compiler first, all eval or dynamic imports will need to be known ahead of time or otherwise have to wait for the compiler before the module can run.