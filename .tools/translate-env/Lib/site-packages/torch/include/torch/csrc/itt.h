#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef ITT_H
#define ITT_H
#include <torch/csrc/utils/pybind.h>

namespace torch::profiler {
void initIttBindings(PyObject* module); // namespace torch::profiler
}
#endif // ITT_H

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
