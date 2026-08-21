#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#ifdef USE_C10D_GLOO

#include <string>

#include <c10/util/Registry.h>
#include <gloo/config.h>
#include <gloo/transport/device.h>

namespace c10d {

class TORCH_API GlooDeviceFactory {
 public:
  // Create new device instance for specific interface.
  static std::shared_ptr<::gloo::transport::Device> makeDeviceForInterface(
      const std::string& interface,
      bool lazyInit);

  // Create new device instance for specific hostname or address.
  static std::shared_ptr<::gloo::transport::Device> makeDeviceForHostname(
      const std::string& hostname,
      bool lazyInit);
};

TORCH_DECLARE_SHARED_REGISTRY(
    GlooDeviceRegistry,
    ::gloo::transport::Device,
    const std::string&, /* interface */
    const std::string&, /* hostname */
    bool /* lazyInit */);

} // namespace c10d

#endif // USE_C10D_GLOO

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
