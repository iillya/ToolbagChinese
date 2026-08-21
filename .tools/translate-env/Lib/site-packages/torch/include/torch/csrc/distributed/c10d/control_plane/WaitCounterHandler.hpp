#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <string>

namespace c10d {
namespace control_plane {

// Returns all wait counter values as a JSON string
std::string getWaitCounterValuesJson();

// Ensures the wait counter backend is registered
void ensureWaitCounterBackendRegistered();

} // namespace control_plane
} // namespace c10d

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
