#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/Export.h>
#include <torch/csrc/profiler/unwind/unwind.h>

namespace torch {
TORCH_API bool get_cpp_stacktraces_enabled();
TORCH_API torch::unwind::Mode get_symbolize_mode();
} // namespace torch

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
