#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Macros.h>
#include <c10/core/GradMode.h>

namespace at {
  using GradMode = c10::GradMode;
  using AutoGradMode = c10::AutoGradMode;
  using NoGradGuard = c10::NoGradGuard;
}

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
