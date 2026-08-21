#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/utils/python_compat.h>
namespace torch::autograd {

PyMethodDef* get_nested_functions_manual();

void initNestedFunctions(PyObject* module);

} // namespace torch::autograd

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
