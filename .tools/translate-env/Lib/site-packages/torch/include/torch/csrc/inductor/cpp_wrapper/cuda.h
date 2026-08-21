#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <torch/csrc/inductor/cpp_wrapper/common.h>
#include <torch/csrc/inductor/cpp_wrapper/device_internal/cuda.h>
#include <torch/csrc/inductor/cpp_wrapper/lazy_triton_compile.h>

#ifdef TORCH_INDUCTOR_PRECOMPILE_HEADERS
#include <ATen/cuda/EmptyTensor.h>
#include <c10/cuda/CUDAGuard.h>
#include <c10/cuda/CUDAStream.h>
#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
