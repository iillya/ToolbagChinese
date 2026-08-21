#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/grad_mode.h>
#include <torch/csrc/Export.h>

namespace torch::autograd {

using GradMode = at::GradMode;
using AutoGradMode = at::AutoGradMode;

} // namespace torch::autograd

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
