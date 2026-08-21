#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/distributed/rpc/message.h>
#include <torch/csrc/distributed/rpc/rpc_command_base.h>

namespace torch::distributed::rpc {

// Return value of a builtin operator or a TorchScript function.
class TORCH_API ScriptResp final : public RpcCommandBase {
 public:
  explicit ScriptResp(at::IValue&& values);

  const at::IValue& value();
  c10::intrusive_ptr<Message> toMessageImpl() && override;
  static std::unique_ptr<ScriptResp> fromMessage(const Message& message);

 private:
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
  const at::IValue value_;
};

} // namespace torch::distributed::rpc

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
