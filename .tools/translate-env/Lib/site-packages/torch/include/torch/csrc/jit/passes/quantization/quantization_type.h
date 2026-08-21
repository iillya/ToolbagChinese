#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <cstdint>
#include <ostream>

namespace torch::jit {

// Quantization type (dynamic quantization, static quantization).
// Should match the Python enum in quantize_jit.py
enum QuantType : std::uint8_t { DYNAMIC = 0, STATIC };

std::ostream& operator<<(std::ostream& os, QuantType t);

} // namespace torch::jit

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
