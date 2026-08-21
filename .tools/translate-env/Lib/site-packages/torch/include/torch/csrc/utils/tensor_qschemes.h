#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <torch/csrc/QScheme.h>

namespace torch::utils {

PyObject* getTHPQScheme(at::QScheme qscheme);
void initializeQSchemes();

} // namespace torch::utils

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
