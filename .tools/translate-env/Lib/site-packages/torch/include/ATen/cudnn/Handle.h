#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/cuda/ATenCUDAGeneral.h>
#include <ATen/cudnn/cudnn-wrapper.h>

namespace at::native {

TORCH_CUDA_CPP_API cudnnHandle_t getCudnnHandle();
} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
