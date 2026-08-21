#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/native/DispatchStub.h>
#include <c10/util/ArrayRef.h>

/*
  Depthwise 3x3 Winograd convolution operator
*/

namespace at {
class Tensor;

namespace native {

using convolution_depthwise3x3_winograd_fn =
    Tensor (*)(const Tensor &, const Tensor &, const Tensor &, IntArrayRef, IntArrayRef, int64_t);

DECLARE_DISPATCH(convolution_depthwise3x3_winograd_fn, convolution_depthwise3x3_winograd_stub)

}  // namespace native
}  // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
