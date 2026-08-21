#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/Context.h>
#include <c10/xpu/XPUFunctions.h>

namespace at::xpu {

inline Device getDeviceFromPtr(void* ptr) {
  auto device = c10::xpu::get_device_idx_from_pointer(ptr);
  return {c10::DeviceType::XPU, device};
}

} // namespace at::xpu

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
