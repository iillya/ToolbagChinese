#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <cstdint>

namespace at {
class TensorBase;

namespace native {

// NOTE: these functions require output tensors to be contiguous
void launch_cummax_cuda_kernel(const TensorBase& self, const TensorBase& values,
                               const TensorBase& indices, int64_t dim);
void launch_cummin_cuda_kernel(const TensorBase& self, const TensorBase& values,
                               const TensorBase& indices, int64_t dim);
void launch_logcumsumexp_cuda_kernel(const TensorBase& result, const TensorBase& self, int64_t dim);
void launch_cumsum_cuda_kernel(const TensorBase& result, const TensorBase& self, int64_t dim);
void launch_cumprod_cuda_kernel(const TensorBase& result, const TensorBase& self, int64_t dim);

}}  // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
