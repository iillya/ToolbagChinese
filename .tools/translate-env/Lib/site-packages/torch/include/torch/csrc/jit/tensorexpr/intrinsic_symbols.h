#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#ifdef TORCH_ENABLE_LLVM
#include <c10/util/ArrayRef.h>

namespace torch {
namespace jit {
namespace tensorexpr {

struct SymbolAddress {
  const char* symbol;
  void* address;

  SymbolAddress(const char* sym, void* addr) : symbol(sym), address(addr) {}
};

c10::ArrayRef<SymbolAddress> getIntrinsicSymbols();

} // namespace tensorexpr
} // namespace jit
} // namespace torch
#endif // TORCH_ENABLE_LLVM

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
