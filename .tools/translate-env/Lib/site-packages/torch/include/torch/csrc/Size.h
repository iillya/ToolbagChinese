#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/Export.h>
#include <torch/csrc/autograd/variable.h>
#include <torch/csrc/python_headers.h>
#include <cstdint>

TORCH_PYTHON_API extern PyTypeObject THPSizeType;

#define THPSize_Check(obj) (Py_TYPE(obj) == &THPSizeType)

PyObject* THPSize_New(const torch::autograd::Variable& t);
PyObject* THPSize_NewFromSizes(int64_t dim, const int64_t* sizes);
PyObject* THPSize_NewFromSymSizes(const at::Tensor& t);

void THPSize_init(PyObject* module);

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
