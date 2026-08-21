#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/nn/options/batchnorm.h>
#include <torch/nn/options/conv.h>
#include <torch/nn/options/dropout.h>
#include <torch/nn/options/fold.h>
#include <torch/nn/options/linear.h>
#include <torch/nn/options/loss.h>
#include <torch/nn/options/normalization.h>
#include <torch/nn/options/padding.h>
#include <torch/nn/options/pixelshuffle.h>
#include <torch/nn/options/pooling.h>
#include <torch/nn/options/rnn.h>
#include <torch/nn/options/transformer.h>
#include <torch/nn/options/transformercoder.h>
#include <torch/nn/options/transformerlayer.h>
#include <torch/nn/options/upsampling.h>
#include <torch/nn/options/vision.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
