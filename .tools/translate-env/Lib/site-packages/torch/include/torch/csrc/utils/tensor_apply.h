#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/Tensor.h>
#include <torch/csrc/python_headers.h>

namespace torch::utils {

const at::Tensor& apply_(const at::Tensor& self, PyObject* fn);
const at::Tensor& map_(
    const at::Tensor& self,
    const at::Tensor& other_,
    PyObject* fn);
const at::Tensor& map2_(
    const at::Tensor& self,
    const at::Tensor& x_,
    const at::Tensor& y_,
    PyObject* fn);

} // namespace torch::utils

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
