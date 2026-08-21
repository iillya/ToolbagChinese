#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <ATen/core/Tensor.h>

namespace at::native {

Tensor& qembeddingbag_byte_prepack_out(
    Tensor& output,
    const Tensor& weight,
    const std::optional<Tensor>& rowwise_min_max_opt = std::nullopt);

Tensor qembeddingbag_byte_prepack(const Tensor& weight);

Tensor qembeddingbag_byte_prepack_meta(const Tensor& weight);

} // namespace at::native

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
