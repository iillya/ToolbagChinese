#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef PTM_COREML_Context_h
#define PTM_COREML_Context_h

#include <string>

namespace torch::jit::mobile::coreml {

struct ContextInterface {
  virtual ~ContextInterface() = default;
  virtual void setModelCacheDirectory(std::string path) = 0;
};

class BackendRegistrar {
 public:
  explicit BackendRegistrar(ContextInterface* ctx);
};

void setModelCacheDirectory(std::string path);

} // namespace torch::jit::mobile::coreml

#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
