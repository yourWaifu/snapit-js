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

class BindingsDefine;
struct NativeFunctionDefine;

struct NativeVFunctionReturnValue {
    /* implicit */ NativeVFunctionReturnValue(hermes::vm::HermesValue&& value_) : status(hermes::vm::ExecutionStatus::RETURNED), value(std::move(value_)) {}
    /* implicit */ NativeVFunctionReturnValue(hermes::vm::ExecutionStatus status_) : status(status_) {}

    static NativeVFunctionReturnValue encodeUndefined() { return hermes::vm::HermesValue::encodeUndefinedValue(); }

    hermes::vm::ExecutionStatus status;
    hermes::vm::HermesValue value;
};

class NativeVFunction {
public:
    NativeVFunction() {};
    using ReturnValue = NativeVFunctionReturnValue;

    virtual ReturnValue invoke(
        const BindingsDefine& context,
        hermes::vm::Runtime& runtime/*,*/
        //hermes::vm::NativeArgs& args
    ) const {
        return hermes::vm::HermesValue::encodeUndefinedValue();
    };

    virtual ~NativeVFunction() {};
private:
};

//wrapper around handle to stop binding errors
struct SymbolIDHandle {
public:
    using type_value = hermes::vm::SymbolID;
    
    SymbolIDHandle(hermes::vm::Handle<hermes::vm::SymbolID> _base): base(_base) {}

    inline hermes::vm::Handle<hermes::vm::SymbolID> unwrapWrapper() const {
        return base;
    }

    inline hermes::vm::SymbolID get() const {
        return base.get();
    }
private:
    hermes::vm::Handle<hermes::vm::SymbolID> base;
};

//wrapper around result to stop binding errors
struct SymbolIDHandleResult {
public:
    SymbolIDHandleResult(hermes::vm::CallResult<hermes::vm::Handle<hermes::vm::SymbolID>> _base): base(std::move(_base)) {}
    
    inline hermes::vm::CallResult<hermes::vm::Handle<hermes::vm::SymbolID>>& unwrapWrapper() {
        return base;
    }
private:
    hermes::vm::CallResult<hermes::vm::Handle<hermes::vm::SymbolID>> base;
};

class BindingsDefine {
public:
    BindingsDefine();

    struct FunctionContext {
        FunctionContext(
            const BindingsDefine& _parent,
            const NativeVFunction& _func
        ): parent(_parent), func(_func) {}
        
        const BindingsDefine& parent;
        const NativeVFunction& func;
    };

    virtual void start() {}

    virtual void install(hermes::vm::Runtime& runtime, const BindingsDefine& bindingsDef) const {
        return;
    };

    inline const FunctionContext makeFunctionContext(const NativeVFunction& _func) const noexcept {
        return FunctionContext{*this, _func};
    }

    inline const FunctionContext& getDefaultFunctionContext() const noexcept {
        // functionContextList is initalized with an default function
        return defaultFunctionContext;
    }

    std::unique_ptr<NativeFunctionDefine> function(
        hermes::vm::Runtime& _runtime,
        hermes::vm::Handle<hermes::vm::JSObject> _parentHandle,
        hermes::vm::Handle<hermes::vm::JSObject> _prototypeObjectHandle
    ) const;

    std::unique_ptr<NativeFunctionDefine> functionWithNullPrototype(
        hermes::vm::Runtime& _runtime,
        hermes::vm::Handle<hermes::vm::JSObject> _parentHandle
    ) const;

    std::unique_ptr<NativeFunctionDefine> functionWithOnlyRuntime(
        hermes::vm::Runtime& _runtime
    ) const;

    inline hermes::vm::IdentifierTable& getIdentifierTable(hermes::vm::Runtime& _runtime) const {
        return _runtime.getIdentifierTable();
    }

    inline hermes::vm::UTF16Ref createUTF16Ref(const std::u16string& str) const {
        return hermes::vm::createUTF16Ref(str.c_str());
    }

    inline hermes::vm::ASCIIRef createASCIIRef(const std::string& str) const {
        return hermes::vm::createASCIIRef(str.c_str());
    }

    inline SymbolIDHandleResult getSymbolHandleUTF16(hermes::vm::Runtime& runtime, hermes::vm::UTF16Ref str) const {
        return runtime.getIdentifierTable().getSymbolHandle(runtime, str);
    }

    inline SymbolIDHandleResult getSymbolHandleASCII(hermes::vm::Runtime& runtime, hermes::vm::ASCIIRef str) const {
        return runtime.getIdentifierTable().getSymbolHandle(runtime, str);
    }

    // to do fix this with generics somehow
    inline SymbolIDHandle ignoreAllocationFailure(hermes::vm::Runtime& runtime, SymbolIDHandleResult res) const {
        return runtime.ignoreAllocationFailure(res.unwrapWrapper());
    }

private:
    NativeVFunction defaultFunction;
    const FunctionContext defaultFunctionContext;
};

bool executeHBCBytecode(
    std::unique_ptr<hermes::Buffer> bytes,
    const std::string sourceURL,
    const BindingsDefine& bindings
);

hermes::vm::CallResult<hermes::vm::HermesValue> callFunctionContext(void *context, hermes::vm::Runtime &runtime, hermes::vm::NativeArgs args);

struct NativeFunctionDefine {
    NativeFunctionDefine(
        hermes::vm::Runtime& _runtime,
        hermes::vm::Handle<hermes::vm::JSObject> _parentHandle,
        const BindingsDefine& _context,
        hermes::vm::Handle<hermes::vm::JSObject> _prototypeObjectHandle
    ):
        runtime(_runtime),
        parentHandle(_parentHandle),
        prototypeObjectHandle(_prototypeObjectHandle),
        functionContext(&(_context.getDefaultFunctionContext()))
    {};
    NativeFunctionDefine() = delete;
    
    // The prototype property will be null
    static inline NativeFunctionDefine withNullPrototype(
        hermes::vm::Runtime& _runtime,
        const BindingsDefine& _context,
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
        const BindingsDefine& _context
    ) {
        return NativeFunctionDefine::withNullPrototype(
            _runtime,
            _context,
            std::move(hermes::vm::Handle<hermes::vm::JSObject>::vmcast(
                &_runtime.functionPrototype
            ))
        );
    }

    void setFunction(
        hermes::vm::SymbolID name_,
        const BindingsDefine::FunctionContext& function,
        unsigned paramCount_
    ) {
        name = name_;
        paramCount = paramCount_;
        functionContext = &function;
    }

    bool define() const {
        auto global = runtime.getGlobal();
        auto normalFlags = hermes::vm::DefinePropertyFlags::getNewNonEnumerableFlags();
        void * context = const_cast<BindingsDefine::FunctionContext*>(functionContext);
        auto nativeFunction = hermes::vm::NativeFunction::create(
            runtime, parentHandle, context, callFunctionContext, name, paramCount,
            prototypeObjectHandle, additionalSlotCount);
        auto res = hermes::vm::JSObject::defineOwnProperty(
            global, runtime, name, normalFlags, nativeFunction);
        return res != hermes::vm::ExecutionStatus::EXCEPTION && *res;
    }
    
    hermes::vm::Runtime& runtime;
    hermes::vm::Handle<hermes::vm::JSObject> parentHandle;
    hermes::vm::SymbolID name;
    unsigned paramCount = 0;
    hermes::vm::Handle<hermes::vm::JSObject> prototypeObjectHandle;
    unsigned additionalSlotCount = 0U;

private:
    const BindingsDefine::FunctionContext* functionContext;
};