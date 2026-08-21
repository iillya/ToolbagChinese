#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/core/Tensor.h>
#include <ATen/cuda/Atomic.cuh>
#include <ATen/cuda/CUDAContext.h>
#include <ATen/TensorUtils.h>

namespace at::native {

Tensor embedding_backward_cuda_kernel(
    const Tensor &grad,
    const Tensor &orig_indices,
    const Tensor &sorted_indices,
    const Tensor &count,
    int64_t num_weights,
    int padding_idx = -1,
    bool mode_mean = false,
    const Tensor &offset2bag = Tensor(),
    const Tensor &bag_size = Tensor(),
    const Tensor &per_sample_weights = Tensor());

} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
