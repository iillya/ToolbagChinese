#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef THP_TYPES_INC
#define THP_TYPES_INC

#include <cstddef>

#ifndef INT64_MAX
#include <cstdint>
#endif

template <typename T>
struct THPTypeInfo {};

#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
