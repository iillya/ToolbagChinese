#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/xpu/XPUGraphsC10Utils.h>

namespace at::xpu {

inline CaptureStatus currentStreamCaptureStatus() {
  return c10::xpu::currentStreamCaptureStatusMayInitCtx();
}

inline void assertNotCapturing(const std::string& attempt) {
  auto status = currentStreamCaptureStatus();
  TORCH_CHECK(
      status == CaptureStatus::Executing,
      attempt,
      " during XPU graph capture. If you need this call to be captured, "
      "please file an issue. "
      "Current xpuStreamCaptureStatus: ",
      status);
}

} // namespace at::xpu

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
