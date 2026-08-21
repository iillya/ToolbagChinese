#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/lazy/core/tensor.h>

namespace torch::lazy {

//////////////////////////////////////////////////////////////////////////////
// ATEN operators follows here, listed in alphabetical order.
//////////////////////////////////////////////////////////////////////////////

void copy_(torch::lazy::LazyTensorPtr& input, torch::lazy::LazyTensorPtr& src);
// Fills the input with the given value.
void fill_(torch::lazy::LazyTensorPtr& input, const at::Scalar& value);

} // namespace torch::lazy

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
