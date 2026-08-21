#if !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
#pragma once

#include <torch/csrc/distributed/rpc/message.h>
#include <torch/csrc/distributed/rpc/rpc_command_base.h>

namespace torch::distributed::autograd {

// Empty response for CleanupAutogradContextReq. Send to acknowledge receipt of
// a CleanupAutogradContextReq.
class TORCH_API CleanupAutogradContextResp : public rpc::RpcCommandBase {
 public:
  CleanupAutogradContextResp() = default;
  // Serialization and deserialization methods.
  c10::intrusive_ptr<rpc::Message> toMessageImpl() && override;
  static std::unique_ptr<CleanupAutogradContextResp> fromMessage(
      const rpc::Message& message);
};

} // namespace torch::distributed::autograd

#else
#error "This file should not be included when either TORCH_STABLE_ONLY or TORCH_TARGET_VERSION is defined."
#endif  // !defined(TORCH_STABLE_ONLY) && !defined(TORCH_TARGET_VERSION)
