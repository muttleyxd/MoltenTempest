#pragma once

#include <Tempest/AbstractGraphicsApi>
#include <vulkan/vulkan.hpp>

namespace Tempest {
namespace Detail {

class VDevice;

class VFence : public AbstractGraphicsApi::Fence {
  public:
    VFence(VDevice& dev);
    VFence(VFence&& other);
    ~VFence() override;

    void operator=(VFence&& other);

    void wait() override;
    void reset() override;

    VkFence impl=VK_NULL_HANDLE;

  private:
    VkDevice device=nullptr;
  };

}}
