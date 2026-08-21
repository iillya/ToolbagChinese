#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/data/samplers/base.h>
#include <torch/serialize/archive.h>

namespace torch::data::samplers {
/// Serializes a `Sampler` into an `OutputArchive`.
template <typename BatchRequest>
serialize::OutputArchive& operator<<(
    serialize::OutputArchive& archive,
    const Sampler<BatchRequest>& sampler) {
  sampler.save(archive);
  return archive;
}

/// Deserializes a `Sampler` from an `InputArchive`.
template <typename BatchRequest>
serialize::InputArchive& operator>>(
    serialize::InputArchive& archive,
    Sampler<BatchRequest>& sampler) {
  sampler.load(archive);
  return archive;
}
} // namespace torch::data::samplers

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
