#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef MetalContext_h
#define MetalContext_h

#include <atomic>

#include <ATen/Tensor.h>

namespace at::metal {

struct MetalInterface {
  virtual ~MetalInterface() = default;
  virtual bool is_metal_available() const = 0;
  virtual at::Tensor& metal_copy_(at::Tensor& self, const at::Tensor& src)
      const = 0;
};

extern std::atomic<const MetalInterface*> g_metal_impl_registry;

class MetalImplRegistrar {
 public:
  explicit MetalImplRegistrar(MetalInterface* /*impl*/);
};

at::Tensor& metal_copy_(at::Tensor& self, const at::Tensor& src);

} // namespace at::metal

namespace at::native {
bool is_metal_available();
} // namespace at::native

#endif /* MetalContext_h */

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
