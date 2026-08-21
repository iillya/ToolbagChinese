#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/native/TensorIterator.h>

namespace at {
struct TensorIteratorBase;

namespace native {
inline namespace CPU_CAPABILITY {

void direct_copy_kernel(TensorIteratorBase &iter);
void copy_kernel(TensorIterator& iter, bool /*non_blocking*/);

}}}  // namespace at::native::CPU_CAPABILITY

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
