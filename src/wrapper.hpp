#pragma once
#include <memory>
#include <string>
#include <vector>
#include <list>

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

class NativeFunction;

class BindingsDefine {
public:
    BindingsDefine();

    struct FunctionContext {
        FunctionContext(
            BindingsDefine& _parent,
            std::shared_ptr<NativeFunction> _func
        ): parent(_parent), func(_func) {}
        
        BindingsDefine& parent;
        std::shared_ptr<NativeFunction> func;
    };

    virtual void install(hermes::vm::Runtime& runtime) const {
        return;
    };

    FunctionContext& addFunction(std::shared_ptr<NativeFunction> function);

    inline FunctionContext& getDefaultFunctionContext() noexcept {
        // functionContextList is initalized with an default function
        return *functionContextList.begin();
    }

private:
    std::list<FunctionContext> functionContextList;
};

bool executeHBCBytecode(
    std::unique_ptr<hermes::Buffer> bytes,
    const std::string sourceURL,
    const BindingsDefine& bindings
);

class NativeFunction {
public:
    virtual hermes::vm::CallResult<hermes::vm::HermesValue> impl(
        BindingsDefine& context,
        hermes::vm::Runtime& runtime,
        hermes::vm::NativeArgs args
    ) const {
        return hermes::vm::HermesValue::encodeUndefinedValue();
    };
};

struct NativeFunctionDefine {
    NativeFunctionDefine(
        hermes::vm::Runtime& _runtime,
        hermes::vm::Handle<hermes::vm::JSObject> _parentHandle,
        BindingsDefine& _context,
        hermes::vm::Handle<hermes::vm::JSObject> _prototypeObjectHandle
    ):
        runtime(_runtime),
        parentHandle(_parentHandle),
        context(_context),
        prototypeObjectHandle(_prototypeObjectHandle),
        functionContext(&(context.getDefaultFunctionContext()))
    {};
    NativeFunctionDefine() = delete;
    
    // The prototype property will be null
    static inline NativeFunctionDefine withNullPrototype(
        hermes::vm::Runtime& _runtime,
        BindingsDefine& _context,
        hermes::vm::Handle<hermes::vm::JSObject> _parentHandle
    ) {
        NativeFunctionDefine target{
            _runtime,
            _parentHandle,
            _context,
            std::move(_runtime.makeNullHandle<hermes::vm::JSObject>())
        };
        return target;
    }

    // The prototype property will be null
    static inline NativeFunctionDefine withNullPrototype(
        hermes::vm::Runtime& _runtime,
        BindingsDefine& _context
    ) {
        return NativeFunctionDefine::withNullPrototype(
            _runtime,
            _context,
            std::move(hermes::vm::Handle<hermes::vm::JSObject>::vmcast(
                &_runtime.functionPrototype
            ))
        );
    }

    inline void setFunction(std::shared_ptr<NativeFunction> function) {
        functionContext = &(context.addFunction(std::move(function)));
        functionPtr = [](
            void *context, hermes::vm::Runtime &runtime, hermes::vm::NativeArgs args
        ) -> hermes::vm::CallResult<hermes::vm::HermesValue> {
            BindingsDefine::FunctionContext& functionContext = 
                *static_cast<BindingsDefine::FunctionContext*>(context);
            return functionContext.func->impl(
                functionContext.parent,
                runtime,
                std::move(args));
        };
    }

    inline hermes::vm::Handle<hermes::vm::NativeFunction> create() {
        return hermes::vm::NativeFunction::create(
            runtime, parentHandle, functionContext, functionPtr, name,
            paramCount, prototypeObjectHandle, additionalSlotCount);
    }
    
    hermes::vm::Runtime& runtime;
    hermes::vm::Handle<hermes::vm::JSObject> parentHandle;
    BindingsDefine& context;
    hermes::vm::NativeFunctionPtr functionPtr;
    hermes::vm::SymbolID name;
    unsigned paramCount;
    hermes::vm::Handle<hermes::vm::JSObject> prototypeObjectHandle;
    unsigned additionalSlotCount = 0U;

private:
    BindingsDefine::FunctionContext* functionContext;
};