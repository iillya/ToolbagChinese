#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#ifndef THP_STORAGE_METHODS_INC
#define THP_STORAGE_METHODS_INC

#include <Python.h>

PyMethodDef* THPStorage_getMethods();

#endif

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
