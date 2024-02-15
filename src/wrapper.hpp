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

bool executeHBCBytecode(std::unique_ptr<hermes::Buffer> bytes, const std::string filename);