#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/ivalue.h>
#include <c10/macros/Macros.h>
#include <functional>

namespace at {

// Launches intra-op parallel task, returns a future
TORCH_API c10::intrusive_ptr<c10::ivalue::Future> intraop_launch_future(
    const std::function<void()>& func);

} // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
