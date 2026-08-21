#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/cuda/CUDAConfig.h>
#include <string>

// AT_USE_JITERATOR(), controls whether we jit some elementwise kernels.
// AT_DISABLE_JITERATOR is set by CMake when jiterator should be off.
// Currently set under USE_ROCM + USE_ASAN, because jiterator JITs kernels
// through hiprtc and we haven't set up an ASAN-aware hiprtc runtime.
#ifdef AT_DISABLE_JITERATOR
#define AT_USE_JITERATOR() false
#else
#define AT_USE_JITERATOR() true
#endif
#define jiterator_stringify(...) std::string(#__VA_ARGS__);

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
