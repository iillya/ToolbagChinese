#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/data/datasets/base.h>
#include <torch/data/datasets/chunk.h>
#include <torch/data/datasets/map.h>
#include <torch/data/datasets/mnist.h>
#include <torch/data/datasets/shared.h>
#include <torch/data/datasets/stateful.h>
#include <torch/data/datasets/tensor.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
