#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/Export.h>
#include <cstddef>

namespace torch::data::samplers {
/// A base class for custom index types.
struct TORCH_API CustomBatchRequest {
  CustomBatchRequest() = default;
  CustomBatchRequest(const CustomBatchRequest&) = default;
  CustomBatchRequest(CustomBatchRequest&&) noexcept = default;
  virtual ~CustomBatchRequest() = default;

  /// The number of elements accessed by this index.
  virtual size_t size() const = 0;
};
} // namespace torch::data::samplers

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
