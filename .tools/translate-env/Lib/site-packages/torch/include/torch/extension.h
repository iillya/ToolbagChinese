#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#ifndef TORCH_INDUCTOR_CPP_WRAPPER
// All pure C++ headers for the C++ frontend.
#include <torch/all.h>
#endif

// Python bindings for the C++ frontend (includes Python.h).
#include <torch/python.h>

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
