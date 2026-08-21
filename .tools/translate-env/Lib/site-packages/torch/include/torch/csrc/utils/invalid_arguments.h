#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/python_headers.h>
#include <string>
#include <vector>

namespace torch {

std::string format_invalid_args(
    PyObject* given_args,
    PyObject* given_kwargs,
    const std::string& function_name,
    const std::vector<std::string>& options);

} // namespace torch

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
