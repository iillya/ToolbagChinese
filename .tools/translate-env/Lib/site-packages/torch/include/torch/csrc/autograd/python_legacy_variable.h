#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

// Instantiates torch._C._LegacyVariableBase, which defines the Python
// constructor (__new__) for torch.autograd.Variable.

#include <torch/csrc/python_headers.h>

namespace torch::autograd {

void init_legacy_variable(PyObject* module);

}

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
