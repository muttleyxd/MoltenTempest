#pragma once

#include <Tempest/AbstractGraphicsApi>
#include <vulkan/vulkan.hpp>

namespace Tempest {
namespace Detail {

class VDevice;

class VSemaphore : public AbstractGraphicsApi::Semaphore {
  public:
    VSemaphore(VDevice& dev);
    VSemaphore(VSemaphore&& other);
    ~VSemaphore();

    void operator=(VSemaphore&& other);
    VkSemaphore impl=VK_NULL_HANDLE;

  private:
    VkDevice    device=nullptr;
  };

}}
