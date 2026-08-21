#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/inductor/aoti_include/common.h>
#include <torch/csrc/inductor/aoti_include/kernel_compile_result.h>
#include <torch/csrc/inductor/cpp_wrapper/device_internal/cuda.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
