#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef THCP_CUDA_MODULE_INC
#define THCP_CUDA_MODULE_INC
#include <torch/csrc/utils/pythoncapi_compat.h>

PyObject* THCPModule_getDevice_wrap(PyObject* self);
PyObject* THCPModule_setDevice_wrap(PyObject* self, PyObject* arg);
PyObject* THCPModule_getDeviceName_wrap(PyObject* self, PyObject* arg);
PyObject* THCPModule_getDriverVersion(PyObject* self);
PyObject* THCPModule_isDriverSufficient(PyObject* self);
PyObject* THCPModule_getCurrentBlasHandle_wrap(PyObject* self);
PyObject* THCPModule_getCurrentSolverHandle_wrap(PyObject* self);

#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
