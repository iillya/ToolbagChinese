#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once
#include <torch/csrc/profiler/orchestration/python_tracer.h>

namespace torch::mtia {
using namespace torch::profiler::impl::python_tracer;

void initMemoryProfiler();

std::unique_ptr<PythonMemoryTracerBase> getMemoryTracer();

class MTIAMemoryProfiler final : public PythonMemoryTracerBase {
 public:
  explicit MTIAMemoryProfiler() = default;
  ~MTIAMemoryProfiler() override = default;
  void start() override;
  void stop() override;
  void export_memory_history(const std::string& path) override;
};

} // namespace torch::mtia

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
