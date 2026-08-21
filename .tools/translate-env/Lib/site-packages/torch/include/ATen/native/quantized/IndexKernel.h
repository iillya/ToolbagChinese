#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/native/DispatchStub.h>
#include <ATen/native/TensorIterator.h>

namespace at::native {
using masked_fill_kernel_quantized_fn = void(*)(TensorIterator& iter, const Scalar& value, double scale, int zero_point);
using index_put_kernel_quantized_fn = void(*)(TensorIterator& iter, IntArrayRef index_size, IntArrayRef index_stride, bool accumulate, double scale, int zero_point);

DECLARE_DISPATCH(masked_fill_kernel_quantized_fn, masked_fill_kernel_quantized_stub)
DECLARE_DISPATCH(index_put_kernel_quantized_fn, index_put_kernel_quantized_stub)


} // at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
