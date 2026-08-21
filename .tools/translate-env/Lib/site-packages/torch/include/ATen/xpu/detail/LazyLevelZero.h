#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/detail/XPUHooksInterface.h>
namespace at::xpu {
// Forward-declares at::xpu::LevelZero
struct LevelZero;

namespace detail {
extern LevelZero lazyLevelZero;
} // namespace detail

} // namespace at::xpu

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
