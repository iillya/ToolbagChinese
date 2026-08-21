#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/jit/frontend/name_mangler.h>
#include <torch/csrc/jit/ir/type_hashing.h>

namespace torch::jit {

/**
 * class TypeNameUniquer
 *
 * Generates a unique name for every type `t` passed in. Types that compare
 * equal with EqualType will receive the same unique name.
 *
 * This is used during Module::save(), to resolve type name collisions during
 * serialization.
 */
class TORCH_API TypeNameUniquer {
 public:
  c10::QualifiedName getUniqueName(c10::ConstNamedTypePtr t);

 private:
  NameMangler mangler_;
  std::unordered_set<c10::QualifiedName> used_names_;
  std::unordered_map<
      c10::ConstNamedTypePtr,
      c10::QualifiedName,
      HashType,
      EqualType>
      name_map_;
};
} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
