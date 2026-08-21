#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/nn/functional/batchnorm.h>
#include <torch/nn/functional/conv.h>
#include <torch/nn/functional/distance.h>
#include <torch/nn/functional/dropout.h>
#include <torch/nn/functional/embedding.h>
#include <torch/nn/functional/fold.h>
#include <torch/nn/functional/instancenorm.h>
#include <torch/nn/functional/linear.h>
#include <torch/nn/functional/loss.h>
#include <torch/nn/functional/normalization.h>
#include <torch/nn/functional/padding.h>
#include <torch/nn/functional/pixelshuffle.h>
#include <torch/nn/functional/pooling.h>
#include <torch/nn/functional/upsampling.h>
#include <torch/nn/functional/vision.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
