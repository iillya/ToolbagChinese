#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <gtest/gtest.h>

#include <c10/util/irange.h>

static inline void initHostData(int* hostData, int numel) {
  for (const auto i : c10::irange(numel)) {
    hostData[i] = i;
  }
}

static inline void clearHostData(int* hostData, int numel) {
  for (const auto i : c10::irange(numel)) {
    hostData[i] = 0;
  }
}

static inline void validateHostData(int* hostData, int numel) {
  for (const auto i : c10::irange(numel)) {
    EXPECT_EQ(hostData[i], i);
  }
}

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
