#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <ATen/DeviceAccelerator.h>
#include <torch/csrc/utils/pybind.h>

namespace torch::accelerator {

void initModule(PyObject* module);

} // namespace torch::accelerator

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
