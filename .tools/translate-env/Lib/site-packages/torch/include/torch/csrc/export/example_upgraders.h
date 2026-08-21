#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

namespace torch::_export {

/// Register example upgraders for the upgrader system for testing.
/// This function demonstrates common upgrade patterns and is primarily
/// used for testing and demonstration purposes.
void registerExampleUpgraders();

/// Deregister example upgraders for the upgrader system for testing.
/// This function cleans up the example upgraders that were registered
/// by registerExampleUpgraders().
void deregisterExampleUpgraders();

} // namespace torch::_export

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
