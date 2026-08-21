#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
// This file is only provided for compatibility and may be removed in future
// versions. Use fmt/base.h if you don't need fmt::format and fmt/format.h
// otherwise.

#include "format.h"

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
