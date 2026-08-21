#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/ir/ir.h>

// Functions used by both encapsulation and conversion.

namespace torch::jit {

struct IndexingPatternFinder {
 public:
  static std::vector<Node*> FetchSliceAndSelect(const Node* node);

 private:
  static bool IsSameSource(const Node* n, const Node* m);
};

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
