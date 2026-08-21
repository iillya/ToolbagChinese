#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/Context.h>
#include <c10/xpu/XPUFunctions.h>
#include <c10/xpu/XPUStream.h>

namespace at::xpu {

// XPU is available if we compiled with XPU.
inline bool is_available() {
  return c10::xpu::device_count() > 0;
}

TORCH_XPU_API DeviceProp* getCurrentDeviceProperties();

TORCH_XPU_API DeviceProp* getDeviceProperties(DeviceIndex device);

TORCH_XPU_API int32_t getGlobalIdxFromDevice(DeviceIndex device);

TORCH_XPU_API bool canDeviceAccessPeer(DeviceIndex device, DeviceIndex peer);

} // namespace at::xpu

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
