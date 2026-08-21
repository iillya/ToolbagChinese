#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/profiler/orchestration/observer.h>

// There are some components which use these symbols. Until we migrate them
// we have to mirror them in the old autograd namespace.

namespace torch::autograd::profiler {
using torch::profiler::impl::ActivityType;
using torch::profiler::impl::getProfilerConfig;
using torch::profiler::impl::ProfilerConfig;
using torch::profiler::impl::profilerEnabled;
using torch::profiler::impl::ProfilerState;
} // namespace torch::autograd::profiler

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
