#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/core/SafePyObject.h>
#include <c10/macros/Macros.h>
#include <unordered_map>

namespace at::impl {

struct TORCH_API ThreadLocalPythonObjects {
  static void set(const std::string& key, std::shared_ptr<SafePyObject> value);
  static const std::shared_ptr<SafePyObject>& get(const std::string& key);
  static bool contains(const std::string& key);
  static void erase(const std::string& key);

  static const ThreadLocalPythonObjects& get_state();
  static void set_state(ThreadLocalPythonObjects state);

 private:
  std::unordered_map<std::string, std::shared_ptr<c10::SafePyObject>> obj_dict_;
};

} // namespace at::impl

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
