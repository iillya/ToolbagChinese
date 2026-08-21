#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <cstdint>
#include <string_view>

namespace c10::monitor::detail {

struct WaitCounterDynamicBackend {
  void* self{nullptr};
  intptr_t (*start)(void* self, int64_t nowUs){nullptr};
  void (*stop)(void* self, int64_t nowUs, intptr_t ctx){nullptr};
  void (*destroy)(void* self){nullptr};
};

using WaitCounterDynamicBackendInit =
    void (*)(WaitCounterDynamicBackend*, const char* key, std::size_t keyLen);

// This name needs to be updated if anything in the API above is changed.
constexpr std::string_view kWaitCounterDynamicBackendInitFn =
    "c10_monitor_wait_counter_dynamic_backend_init_v1";
} // namespace c10::monitor::detail

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
