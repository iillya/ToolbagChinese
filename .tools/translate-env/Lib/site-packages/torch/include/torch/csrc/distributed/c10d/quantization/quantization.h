#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
// Copyright (c) Meta Platforms, Inc. and affiliates.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <ATen/ATen.h>

namespace torch::distributed::c10d::quantization {

at::Tensor _float_to_bfloat16_cpu(const at::Tensor& input);
at::Tensor _bfloat16_to_float_cpu(const at::Tensor& input);

} // namespace torch::distributed::c10d::quantization

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
