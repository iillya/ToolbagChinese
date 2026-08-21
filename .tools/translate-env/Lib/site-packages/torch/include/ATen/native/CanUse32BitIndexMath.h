#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/macros/Export.h>
#include <limits>

namespace at {
class TensorBase;
}

namespace at::native {

TORCH_API bool canUse32BitIndexMath(const at::TensorBase &t, int64_t max_elem=std::numeric_limits<int32_t>::max());

}

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
