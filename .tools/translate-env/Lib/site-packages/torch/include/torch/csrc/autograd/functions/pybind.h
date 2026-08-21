#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <torch/csrc/python_headers.h>
#include <torch/csrc/utils/pybind.h>

#include <torch/csrc/autograd/python_cpp_function.h>
#include <torch/csrc/autograd/python_function.h>

// NOLINTNEXTLINE(misc-unused-alias-decls)
namespace py = pybind11;

namespace pybind11::detail {} // namespace pybind11::detail

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
