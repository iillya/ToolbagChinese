#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/serialize/archive.h>
#include <torch/types.h>

namespace torch {
inline serialize::OutputArchive& operator<<(
    serialize::OutputArchive& archive,
    const Tensor& tensor) {
  archive.write("0", tensor);
  return archive;
}

inline serialize::InputArchive& operator>>(
    serialize::InputArchive& archive,
    Tensor& tensor) {
  archive.read("0", tensor);
  return archive;
}
} // namespace torch

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
