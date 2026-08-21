#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/util/Exception.h>

#include <ostream>
#include <string>

namespace at {

enum class LinalgBackend : int8_t { Default, Cusolver, Magma };

inline std::string LinalgBackendToString(at::LinalgBackend backend) {
  switch (backend) {
    case LinalgBackend::Default:
      return "at::LinalgBackend::Default";
    case LinalgBackend::Cusolver:
      return "at::LinalgBackend::Cusolver";
    case LinalgBackend::Magma:
      return "at::LinalgBackend::Magma";
    default:
      TORCH_CHECK(false, "Unknown linalg backend");
  }
}

inline std::ostream& operator<<(
    std::ostream& stream,
    at::LinalgBackend backend) {
  return stream << LinalgBackendToString(backend);
}

} // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
