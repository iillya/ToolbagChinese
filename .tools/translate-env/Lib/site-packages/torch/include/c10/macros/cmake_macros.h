#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
// This file exists for backwards compatibility and has been moved to
// torch/headeronly/macros/cmake_macros.h.in. No end user library should be
// including this file directly anyway (cuz they should be including
// Macros.h instead).
#include <torch/headeronly/macros/cmake_macros.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
