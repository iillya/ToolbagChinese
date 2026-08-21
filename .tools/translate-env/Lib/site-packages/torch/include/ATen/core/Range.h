#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <cstdint>
#include <iosfwd>

namespace at {

struct Range {
  Range(int64_t begin, int64_t end)
    : begin(begin)
    , end(end) {}

  int64_t size() const { return end - begin; }

  Range operator/(int64_t divisor) {
    return Range(begin / divisor, end / divisor);
  }

  int64_t begin;
  int64_t end;
};

std::ostream& operator<<(std::ostream& out, const Range& range);

}  // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
