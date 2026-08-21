#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <ATen/MapAllocator.h>

#ifdef __cplusplus

#ifdef SHM_EXPORTS
#define SHM_API __declspec(dllexport)
#else
#define SHM_API __declspec(dllimport)
#endif

SHM_API void libshm_init(const char* manager_exec_path);

class SHM_API THManagedMapAllocator : public at::RefcountedMapAllocator {
 public:
  THManagedMapAllocator(
      const char* manager_handle,
      const char* filename,
      int flags,
      size_t size)
      : at::RefcountedMapAllocator(filename, flags, size) {}

  static at::DataPtr makeDataPtr(
      const char* manager_handle,
      const char* filename,
      int flags,
      size_t size);
  static THManagedMapAllocator* fromDataPtr(const at::DataPtr&);

  const char* manager_handle() const {
    return "no_manager";
  }
};

#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
