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
        hermes::vm::Runtime& runtime,
        hermes::vm::NativeArgs& args
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

struct StringPrimitiveHandle {
public:
    using type_value = hermes::vm::StringPrimitive;
    
    StringPrimitiveHandle(hermes::vm::Handle<hermes::vm::StringPrimitive> _base): base(_base) {}

    inline hermes::vm::Handle<hermes::vm::StringPrimitive> unwrapWrapper() const {
        return base;
    }

    inline hermes::vm::StringView createStringView(hermes::vm::Runtime& runtime) const {
        return hermes::vm::StringPrimitive::createStringView(runtime, base);
    }

private:
    hermes::vm::Handle<hermes::vm::StringPrimitive> base;
};

struct BigIntPrimitiveHandle {
public:
    using type_value = hermes::vm::BigIntPrimitive;

    BigIntPrimitiveHandle(hermes::vm::Handle<hermes::vm::BigIntPrimitive> _base): base(_base) {}

    inline hermes::vm::Handle<hermes::vm::BigIntPrimitive> unwrapWrapper() const {
        return base;
    }

    hermes::vm::BigIntPrimitive& view() const {
        return *base.get();
    }

private:
    hermes::vm::Handle<hermes::vm::BigIntPrimitive> base;
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

struct Value;
struct Handle;

template<typename T>
hermes::vm::Handle<T> castHandle(const Handle& handle) /*unsafe*/ {
    return hermes::vm::Handle<T>::vmcast(handle.base);
}

template<typename T>
bool isAHandle(hermes::vm::HermesValue value) {
    return hermes::vm::vmisa<T>(value);
}

struct Handle {
public:
    // wrapper for handle and rust will handle the template parts
    Handle(hermes::vm::HandleBase&& other) : base(std::move(other)) {}

    bool isASymbolIDHandle() const { return getValue().isSymbol(); }
    bool isAStringPrimitiveHandle() const { return isAHandle<hermes::vm::StringPrimitive>(getValue()); }
    bool isABigIntPrimitiveHandle() const { return isAHandle<hermes::vm::BigIntPrimitive>(getValue()); }
    // the following need to be used with option in Rust to make them safe
    hermes::vm::HermesValue getValue() const { return base.getHermesValue(); }
    SymbolIDHandle castToSymbolIDHandle(void*) /*unsafe*/ const { return castHandle<hermes::vm::SymbolID>(*this); }
    StringPrimitiveHandle castToStringPrimitiveHandle(void*) /*unsafe*/ const { return castHandle<hermes::vm::StringPrimitive>(*this); }
    BigIntPrimitiveHandle castToBigIntPrimitiveHandle(void*) /*unsafe*/ const { return castHandle<hermes::vm::BigIntPrimitive>(*this); }

    hermes::vm::HandleBase base;
private:
};

struct Value {
public:

    Value(hermes::vm::HermesValue&& other) : base(std::move(other)) {}

    static hermes::vm::HermesValue encodeUndefined() { return hermes::vm::HermesValue::encodeUndefinedValue(); }
    static hermes::vm::HermesValue encodeNaNValue() { return hermes::vm::HermesValue::encodeNaNValue(); }

    inline Value updatePointer(void* ptr) /*unsafe*/ const { return base.updatePointer(ptr); }
    inline void setPointer(void* ptr) /*unsafe*/ { return base.unsafeUpdatePointer(ptr); }

    inline bool isNull() const { return base.isNull(); }
    inline bool isUndefined() const { return base.isUndefined(); }
    inline bool isEmpty() const { return base.isEmpty(); }
    inline bool isNativeValue() const { return base.isNativeValue(); }
    inline bool isSymbol() const { return base.isSymbol(); }
    inline bool isBool() const { return base.isBool(); }
    inline bool isObject() const { return base.isObject(); }
    inline bool isString() const { return base.isString(); }
    inline bool isBigInt() const { return base.isBigInt(); }
    inline bool isDouble() const { return base.isDouble(); }
    inline bool isPointer() const { return base.isPointer(); }
    inline bool isNumber() const { return base.isNumber(); }
    inline hermes::vm::HermesValue::RawType getRaw() const { return base.getRaw(); }
    // the following need to be used with option in Rust to make them safe
    inline void* getPointer() /*unsafe*/ const { return base.getPointer(); }
    inline double getDouble(void*) /*unsafe*/ const { return base.getDouble(); }
    inline uint32_t getNativeUInt32(void*) /*unsafe*/ const { return base.getNativeUInt32(); }
    //inline hermes::vm::SymbolID getSymbol(void*) /*unsafe*/ const { return base.getSymbol(); }
    inline bool getBool(void*) /*unsafe*/ const { return base.getBool(); }
    //inline hermes::vm::StringPrimitive /*unsafe*/ getString(void*) const { return base.getString(); }
    //inline hermes::vm::BigIntPrimitive /*unsafe*/ getBigInt(void*) const { return base.getBigInt(); }
    //inline void* getObject(void*) /*unsafe*/ const { return base.getObject() }
    inline double getNumber(void*) /*unsafe*/ const { return base.getNumber(); }

    hermes::vm::HermesValue base;
private:
};

struct Arguments {
public:
    inline static unsigned getArgCount(const hermes::vm::NativeArgs& base) {
        return base.getArgCount();
    }

    // can't compile, needs a wrapper for ConstArgIterator or reverse_iterator
    /*inline static hermes::vm::ConstArgIterator begin(const hermes::vm::NativeArgs& base) {
        return base.begin();
    }*/

    // getArg is safe as it return undefined if out of range
    inline static Value getArg(const hermes::vm::NativeArgs& base, unsigned index) {
        return std::move(base.getArg(index));
    }

    // getArgHandle is safe as it return undefined handle if out of range
    inline static Handle getArgHandle(const hermes::vm::NativeArgs& base, unsigned index) {
        return std::move(base.getArgHandle(index));
    }
    
private:
};
