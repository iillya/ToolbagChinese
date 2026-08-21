#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <cstddef>
#include <cstdint>

#include "c10/macros/Macros.h"


namespace caffe2::serialize {

// this is the interface for the (file/stream/memory) reader in
// PyTorchStreamReader. with this interface, we can extend the support
// besides standard istream
class TORCH_API ReadAdapterInterface {
 public:
  virtual size_t size() const = 0;
  virtual size_t read(uint64_t pos, void* buf, size_t n, const char* what = "")
      const = 0;
  virtual ~ReadAdapterInterface();
};

} // namespace caffe2::serialize

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
