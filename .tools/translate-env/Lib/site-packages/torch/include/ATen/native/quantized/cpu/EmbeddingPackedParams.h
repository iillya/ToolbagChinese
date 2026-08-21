#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/core/Tensor.h>
#include <ATen/core/ivalue.h>

struct EmbeddingPackedParamsBase : public torch::jit::CustomClassHolder {
  virtual at::Tensor embeddingbag_byte(
    const at::Tensor& indices,
    const std::optional<at::Tensor>& offsets,
    bool pruned_weights,
    const std::optional<at::Tensor>& per_sample_weights_,
    const std::optional<at::Tensor>& compressed_indices_mapping,
    bool include_last_offset,
    bool is_embedding_op) = 0;

  virtual at::Tensor embeddingbag_4bit(
    const at::Tensor& indices,
    const std::optional<at::Tensor>& offsets,
    bool pruned_weights,
    const std::optional<at::Tensor>& per_sample_weights_,
    const std::optional<at::Tensor>& compressed_indices_mapping,
    bool include_last_offset,
    bool is_embedding_op) = 0;

  virtual at::Tensor unpack() = 0;

  virtual int64_t bit_rate() const = 0;
  virtual int64_t version() const = 0;
};

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
