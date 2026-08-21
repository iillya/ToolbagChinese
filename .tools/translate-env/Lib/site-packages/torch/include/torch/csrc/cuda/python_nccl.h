#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/python_headers.h>

PyObject* THCPModule_nccl_version(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_version_suffix(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_unique_id(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_init_rank(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_reduce(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_all_reduce(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_broadcast(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_all_gather(PyObject* self, PyObject* args);
PyObject* THCPModule_nccl_reduce_scatter(PyObject* self, PyObject* args);

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
