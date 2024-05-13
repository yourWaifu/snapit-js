#include "wrapper.hpp"

BindingsDefine::BindingsDefine():
  defaultFunctionContext({*this, defaultFunction})
{
  start();
}

std::unique_ptr<NativeFunctionDefine> BindingsDefine::function(
  hermes::vm::Runtime& _runtime,
  hermes::vm::Handle<hermes::vm::JSObject> _parentHandle,
  hermes::vm::Handle<hermes::vm::JSObject> _prototypeObjectHandle
) const {
  return std::make_unique<::NativeFunctionDefine>(
    _runtime, _parentHandle, *this, _prototypeObjectHandle
  );
}

std::unique_ptr<NativeFunctionDefine> BindingsDefine::functionWithNullPrototype(
    hermes::vm::Runtime& _runtime,
    hermes::vm::Handle<hermes::vm::JSObject> _parentHandle
) const {
  return function(_runtime, _parentHandle, _runtime.makeNullHandle<hermes::vm::JSObject>());
}

std::unique_ptr<NativeFunctionDefine> BindingsDefine::functionWithOnlyRuntime(
    hermes::vm::Runtime& _runtime
) const {
  return functionWithNullPrototype(_runtime, hermes::vm::Handle<hermes::vm::JSObject>::vmcast(
      &_runtime.functionPrototype
  ));
}

hermes::vm::CallResult<hermes::vm::HermesValue> callFunctionContext(void *context, hermes::vm::Runtime &runtime, hermes::vm::NativeArgs args)
{
  BindingsDefine::FunctionContext& functionContext = 
      *static_cast<BindingsDefine::FunctionContext*>(context);
  /*return */functionContext.func.invoke(
      functionContext.parent,
      runtime,
      args);
  return hermes::vm::HermesValue::encodeUndefinedValue();
}

hermes::vm::RuntimeConfig makeRuntimeConfig()
{
  // to do, make options for these
  return hermes::vm::RuntimeConfig::Builder()
      .withGCConfig(hermes::vm::GCConfig::Builder()
                        .withInitHeapSize(1024 * 1024)
                        .withMaxHeapSize(1024 * 1024 * 1024)
                        .withSanitizeConfig(
                            hermes::vm::GCSanitizeConfig::Builder()
                                .withSanitizeRate(0.1)
                                .withRandomSeed(-1)
                                .build())
                        .withShouldRecordStats(false)
                        .withShouldReleaseUnused(hermes::vm::kReleaseUnusedOld)
                        .withName("hvm-rust")
                        .build())
      .withEnableBlockScoping(true)
      .withES6Promise(true)
      .withES6Proxy(true)
      .withIntl(true)
      .withMicrotaskQueue(false)
      .withTrackIO(true)
      .withEnableHermesInternal(true)
      .withEnableHermesInternalTestMethods(true)
      .withMaxNumRegisters(1024 * 1024)
      .build();
}

struct ExecuteOptions {
  ExecuteOptions(): runtimeConfig(makeRuntimeConfig()) {}

  hermes::vm::RuntimeConfig runtimeConfig;

    // execution options
  // to do, make options for these
  /// Enable basic block profiling.
  bool basicBlockProfiling{false};

  /// Stop after creating the RuntimeModule.
  bool stopAfterInit{false};

  /// Execution time limit.
  uint32_t timeLimit{0};

  /// Perform a full GC just before printing any statistics.
  bool forceGCBeforeStats{false};

  /// Try to execute the same number of CPU instructions
  /// across repeated invocations of the same JS.
  bool stabilizeInstructionCount{false};

  /// Run the sampling profiler.
  bool sampleProfiling{false};

  /// Start tracking heap objects before executing bytecode.
  bool heapTimeline{false};
};

bool executeHBCBytecode(
  std::unique_ptr<hermes::Buffer> bytes,
  const std::string sourceName,
  const BindingsDefine& bindings
) {
  using namespace hermes;
  std::shared_ptr<hbc::BCProvider> bytecode = std::move(
    hbc::BCProviderFromBuffer::createBCProviderFromBuffer(
      std::move(bytes)
    ).first);
  if (!bytecode) {
    return false;
  }
    
  const ExecuteOptions options = ExecuteOptions{};
  const std::string *filename = &sourceName;

  bool shouldRecordGCStats =
      options.runtimeConfig.getGCConfig().getShouldRecordStats();
  if (shouldRecordGCStats) {
    vm::instrumentation::PerfEvents::begin();
  }

  std::unique_ptr<vm::StatSamplingThread> statSampler;
  auto runtime = vm::Runtime::create(options.runtimeConfig);

  if (options.timeLimit > 0) {
    runtime->timeLimitMonitor = vm::TimeLimitMonitor::getOrCreate();
    runtime->timeLimitMonitor->watchRuntime(
        *runtime, std::chrono::milliseconds(options.timeLimit));
  }

  if (shouldRecordGCStats) {
    statSampler = std::make_unique<vm::StatSamplingThread>(
        std::chrono::milliseconds(100));
  }

  if (options.heapTimeline) {
#ifdef HERMES_MEMORY_INSTRUMENTATION
    runtime->enableAllocationLocationTracker();
#else
    llvh::errs() << "Failed to track allocation locations; build does not"
                    "include memory instrumentation\n";
#endif
  }

  vm::GCScope scope(*runtime);

  bindings.install(*runtime, bindings);

  vm::RuntimeModuleFlags flags;
  flags.persistent = true;

  if (options.stopAfterInit) {
    vm::Handle<vm::Domain> domain =
        runtime->makeHandle(vm::Domain::create(*runtime));
    if (LLVM_UNLIKELY(
            vm::RuntimeModule::create(
                *runtime,
                domain,
                facebook::hermes::debugger::kInvalidLocation,
                std::move(bytecode),
                flags) == vm::ExecutionStatus::EXCEPTION)) {
      llvh::errs() << "Failed to initialize main RuntimeModule\n";
      return false;
    }

    return true;
  }

#if HERMESVM_SAMPLING_PROFILER_AVAILABLE
  if (options.sampleProfiling) {
    vm::SamplingProfiler::enable();
  }
#endif // HERMESVM_SAMPLING_PROFILER_AVAILABLE

  llvh::StringRef sourceURL{};
  if (filename)
    sourceURL = *filename;
  vm::CallResult<vm::HermesValue> status = runtime->runBytecode(
      std::move(bytecode),
      flags,
      sourceURL,
      vm::Runtime::makeNullHandle<vm::Environment>());

#if HERMESVM_SAMPLING_PROFILER_AVAILABLE
  if (options.sampleProfiling) {
    vm::SamplingProfiler::disable();
    vm::SamplingProfiler::dumpChromeTraceGlobal(llvh::errs());
  }
#endif // HERMESVM_SAMPLING_PROFILER_AVAILABLE

  bool threwException = status == vm::ExecutionStatus::EXCEPTION;

  if (threwException) {
    // Make sure stdout catches up to stderr.
    llvh::outs().flush();
    runtime->printException(
        llvh::errs(), runtime->makeHandle(runtime->getThrownValue()));
  }

  // to do implement microtask
  /*
  // Perform a microtask checkpoint after running script.
  microtask::performCheckpoint(*runtime);

  if (!ctx.tasksEmpty()) {
    vm::GCScopeMarkerRAII marker{scope};
    // Run the tasks until there are no more.
    vm::MutableHandle<vm::Callable> task{*runtime};
    while (auto optTask = ctx.dequeueTask()) {
      task = std::move(*optTask);
      auto callRes = vm::Callable::executeCall0(
          task, *runtime, vm::Runtime::getUndefinedValue(), false);
      if (LLVM_UNLIKELY(callRes == vm::ExecutionStatus::EXCEPTION)) {
        threwException = true;
        llvh::outs().flush();
        runtime->printException(
            llvh::errs(), runtime->makeHandle(runtime->getThrownValue()));
        break;
      }

      // Perform a microtask checkpoint at the end of every task tick.
      microtask::performCheckpoint(*runtime);
    }
  }
  */

#ifdef HERMESVM_PROFILER_OPCODE
  runtime->dumpOpcodeStats(llvh::outs());
#endif

#ifdef HERMESVM_PROFILER_JSFUNCTION
  runtime->dumpJSFunctionStats();
#endif

#ifdef HERMESVM_PROFILER_NATIVECALL
  runtime->dumpNativeCallStats(llvh::outs());
#endif

  if (shouldRecordGCStats) {
    llvh::errs() << "Process stats:\n";
    statSampler->stop().printJSON(llvh::errs());

    if (options.forceGCBeforeStats) {
      runtime->collect("forced for stats");
    }
    // to do implement printStats
    //printStats(*runtime, llvh::errs());
  }

#ifdef HERMESVM_PROFILER_BB
  if (options.basicBlockProfiling) {
    runtime->getBasicBlockExecutionInfo().dump(llvh::errs());
  }
#endif

  return !threwException;
}