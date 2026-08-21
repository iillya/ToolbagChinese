#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef THCP_GDSFILE_INC
#define THCP_GDSFILE_INC

#include <torch/csrc/python_headers.h>

namespace torch::cuda::shared {
void initGdsBindings(PyObject* module);
}
#endif // THCP_GDSFILE_INC

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
