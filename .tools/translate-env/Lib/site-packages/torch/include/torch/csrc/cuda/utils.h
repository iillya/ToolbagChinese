#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/cuda/CUDAStream.h>
#include <torch/csrc/utils/python_numbers.h>

#include <vector>

std::vector<std::optional<at::cuda::CUDAStream>>
THPUtils_PySequence_to_CUDAStreamList(PyObject* obj);

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
