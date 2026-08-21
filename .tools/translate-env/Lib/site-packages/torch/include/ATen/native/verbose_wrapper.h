#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Export.h>

namespace torch::verbose {
TORCH_API int _mkl_set_verbose(int enable);
TORCH_API int _mkldnn_set_verbose(int level);
} // namespace torch::verbose

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
