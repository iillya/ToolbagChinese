#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <cstdint>

enum class MobileOptimizerType : int8_t {
  CONV_BN_FUSION,
  INSERT_FOLD_PREPACK_OPS,
  REMOVE_DROPOUT,
  FUSE_ADD_RELU,
  HOIST_CONV_PACKED_PARAMS,
  CONV_1D_TO_2D,
  VULKAN_AUTOMATIC_GPU_TRANSFER,
};

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
