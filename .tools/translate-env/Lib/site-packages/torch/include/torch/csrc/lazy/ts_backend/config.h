#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <c10/util/Flags.h>

// TODO(whc) unclear if this is useful, has only been tested as true
TORCH_DECLARE_bool(torch_lazy_ts_tensor_update_sync);

TORCH_DECLARE_bool(torch_lazy_ts_cuda);

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
