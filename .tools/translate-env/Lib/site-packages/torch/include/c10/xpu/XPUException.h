#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/util/Exception.h>
#include <sycl/sycl.hpp>

namespace c10::xpu {

static inline sycl::async_handler asyncHandler =
    [](const sycl::exception_list& el) {
      if (el.size() == 0) {
        return;
      }
      for (const auto& e : el) {
        try {
          std::rethrow_exception(e);
        } catch (sycl::exception& e) {
          TORCH_WARN("SYCL Exception: ", e.what());
        }
      }
      throw;
    };

} // namespace c10::xpu

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
