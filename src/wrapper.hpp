#pragma once
#include <memory>
#include <string>
#include <vector>

#include "hermes/BCGen/HBC/BytecodeDataProvider.h"
#include "hermes/Public/RuntimeConfig.h"
#include "hermes/VM/instrumentation/StatSamplingThread.h"

#include "hermes/CompilerDriver/CompilerDriver.h"
#include "hermes/Support/MemoryBuffer.h"
#include "hermes/Support/UTF8.h"
#include "hermes/VM/Callable.h"
#include "hermes/VM/Domain.h"
#include "hermes/VM/JSObject.h"
#include "hermes/VM/MockedEnvironment.h"
#include "hermes/VM/NativeArgs.h"
#include "hermes/VM/Profiler/SamplingProfiler.h"
#include "hermes/VM/Runtime.h"
#include "hermes/VM/StringPrimitive.h"
#include "hermes/VM/StringView.h"
#include "hermes/VM/TimeLimitMonitor.h"
#include "hermes/VM/instrumentation/PerfEvents.h"

#include "hermes/BCGen/HBC/BytecodeStream.h"
#include "hermes/BCGen/HBC/HBC.h"
#include "hermes/VM/Runtime.h"

class BindingsDefine {
public:
    virtual void install() const {};
};

bool executeHBCBytecode(
    std::unique_ptr<hermes::Buffer> bytes,
    const std::string sourceURL,
    const BindingsDefine& bindings
);

struct NativeFunctionDefine {
    NativeFunctionDefine(
        hermes::vm::Runtime& _runtime,
        hermes::vm::Handle<hermes::vm::JSObject> _parentHandle,
        hermes::vm::Handle<hermes::vm::JSObject> _prototypeObjectHandle
    ):
        runtime(_runtime),
        parentHandle(_parentHandle),
        prototypeObjectHandle(_prototypeObjectHandle)
    {};
    NativeFunctionDefine() = delete;
    
    // The prototype property will be null
    static inline NativeFunctionDefine withNullPrototype(hermes::vm::Runtime& _runtime, hermes::vm::Handle<hermes::vm::JSObject> _parentHandle) {
        NativeFunctionDefine target{
            _runtime,
            _parentHandle,
            std::move(_runtime.makeNullHandle<hermes::vm::JSObject>())
        };
        return target;
    }

    // The prototype property will be null
    static inline NativeFunctionDefine withNullPrototype(hermes::vm::Runtime& _runtime) {
        return NativeFunctionDefine::withNullPrototype(
            _runtime,
            std::move(hermes::vm::Handle<hermes::vm::JSObject>::vmcast(&_runtime.functionPrototype))
        );
    }

    inline hermes::vm::Handle<hermes::vm::NativeFunction> create() {
        return hermes::vm::NativeFunction::create(runtime, parentHandle, context, functionPtr, name, paramCount, prototypeObjectHandle, additionalSlotCount);
    }
    
    hermes::vm::Runtime& runtime;
    hermes::vm::Handle<hermes::vm::JSObject> parentHandle;
    void *context = nullptr;
    hermes::vm::NativeFunctionPtr functionPtr;
    hermes::vm::SymbolID name;
    unsigned paramCount;
    hermes::vm::Handle<hermes::vm::JSObject> prototypeObjectHandle;
    unsigned additionalSlotCount = 0U;
};