#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#include <c10/core/ScalarType.h>
#import <nlohmann/json.hpp>

#include <string>

namespace torch::jit::mobile::coreml {

struct TensorSpec {
  std::string name;
  c10::ScalarType dtype = c10::ScalarType::Float;
};

static inline c10::ScalarType scalar_type(const std::string& type_string) {
  if (type_string == "0") {
    return c10::ScalarType::Float;
  } else if (type_string == "1") {
    return c10::ScalarType::Double;
  } else if (type_string == "2") {
    return c10::ScalarType::Int;
  } else if (type_string == "3") {
    return c10::ScalarType::Long;
  }
  return c10::ScalarType::Undefined;
}

} // namespace torch::jit::mobile::coreml

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
