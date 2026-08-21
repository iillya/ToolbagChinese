#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <ATen/native/DispatchStub.h>
#include <c10/core/Scalar.h>

namespace at {
struct TensorIterator;

namespace native {

DECLARE_DISPATCH(void(*)(TensorIterator&, const Scalar&, const Scalar&, const Scalar&), arange_stub)
DECLARE_DISPATCH(void(*)(TensorIterator&, const Scalar&, const Scalar&, int64_t), linspace_stub)

}}  // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
