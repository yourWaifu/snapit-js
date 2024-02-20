#include "wrapper.hpp"

BindingsDefine::FunctionContext& BindingsDefine::addFunction(std::shared_ptr<NativeFunction> ptr) {
  return functionContextList.emplace_back(*this, ptr);
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
  
  vm::RuntimeConfig runtimeConfig = makeRuntimeConfig();
  bool shouldRecordGCStats =
    runtimeConfig.getGCConfig().getShouldRecordStats();
  if (shouldRecordGCStats) {
    vm::instrumentation::PerfEvents::begin();
  }

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

  std::unique_ptr<vm::StatSamplingThread> statSampler;
  auto runtime = vm::Runtime::create(runtimeConfig);
  if (stabilizeInstructionCount) {
    // Try to limit features that can introduce unpredictable CPU instruction
    // behavior. Date is a potential cause, but is not handled currently.
    vm::MockedEnvironment env;
    env.stabilizeInstructionCount = true;
    runtime->setMockedEnvironment(env);
  }

  if (timeLimit > 0) {
    runtime->timeLimitMonitor = vm::TimeLimitMonitor::getOrCreate();
    runtime->timeLimitMonitor->watchRuntime(
        *runtime, std::chrono::milliseconds(timeLimit));
  }

  if (shouldRecordGCStats) {
    statSampler = std::make_unique<vm::StatSamplingThread>(
        std::chrono::milliseconds(100));
  }

  if (heapTimeline) {
#ifdef HERMES_MEMORY_INSTRUMENTATION
    runtime->enableAllocationLocationTracker();
#else
    llvh::errs() << "Failed to track allocation locations; build does not"
                    "include memory instrumentation\n";
#endif
  }

  vm::GCScope scope(*runtime);

  bindings.install(*runtime);

  vm::RuntimeModuleFlags flags;
  flags.persistent = true;

  if (stopAfterInit) {
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
  if (sampleProfiling) {
    vm::SamplingProfiler::enable();
  }
#endif // HERMESVM_SAMPLING_PROFILER_AVAILABLE

  llvh::StringRef sourceURL{};
  if (!sourceName.empty())
    sourceURL = sourceName;
  vm::CallResult<vm::HermesValue> status = runtime->runBytecode(
      std::move(bytecode),
      flags,
      sourceURL,
      vm::Runtime::makeNullHandle<vm::Environment>());

#if HERMESVM_SAMPLING_PROFILER_AVAILABLE
  if (sampleProfiling) {
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

    if (forceGCBeforeStats) {
      runtime->collect("forced for stats");
    }
    // to do implement printStats
    //printStats(*runtime, llvh::errs());
  }

#ifdef HERMESVM_PROFILER_BB
  if (basicBlockProfiling) {
    runtime->getBasicBlockExecutionInfo().dump(llvh::errs());
  }
#endif

  return !threwException;
}