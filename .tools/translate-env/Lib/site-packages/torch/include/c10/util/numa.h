#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <c10/macros/Export.h>
#include <c10/util/Flags.h>
#include <cstddef>

C10_DECLARE_bool(caffe2_cpu_numa_enabled);

namespace c10 {

/**
 * Check whether NUMA is enabled
 */
C10_API bool IsNUMAEnabled();

/**
 * Bind to a given NUMA node
 */
C10_API void NUMABind(int numa_node_id);

/**
 * Get the NUMA id for a given pointer `ptr`
 */
C10_API int GetNUMANode(const void* ptr);

/**
 * Get number of NUMA nodes
 */
C10_API int GetNumNUMANodes();

/**
 * Move the memory pointed to by `ptr` of a given size to another NUMA node
 */
C10_API void NUMAMove(void* ptr, size_t size, int numa_node_id);

/**
 * Get the current NUMA node id
 */
C10_API int GetCurrentNUMANode();

} // namespace c10

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
