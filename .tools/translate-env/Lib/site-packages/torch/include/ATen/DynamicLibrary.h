#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/Utils.h>
#include <c10/macros/Export.h>
#include <c10/util/Exception.h>

namespace c10 {

class DynamicLibraryError : public Error {
  using Error::Error;
};

} // namespace c10

namespace at {

struct DynamicLibrary {
  AT_DISALLOW_COPY_AND_ASSIGN(DynamicLibrary);
  DynamicLibrary(DynamicLibrary&& other) = delete;
  DynamicLibrary& operator=(DynamicLibrary&&) = delete;

  TORCH_API DynamicLibrary(
      const char* name,
      const char* alt_name = nullptr,
      bool leak_handle = false);

  TORCH_API void* sym(const char* name);

  TORCH_API ~DynamicLibrary();

 private:
  bool leak_handle;
  void* handle = nullptr;
};

} // namespace at

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
