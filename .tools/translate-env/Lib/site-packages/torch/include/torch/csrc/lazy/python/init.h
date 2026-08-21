#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <pybind11/pybind11.h>
#include <torch/csrc/Export.h>
#include <torch/csrc/utils/pybind.h>

namespace torch::lazy {

TORCH_PYTHON_API void initLazyBindings(PyObject* module);

} // namespace torch::lazy

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
