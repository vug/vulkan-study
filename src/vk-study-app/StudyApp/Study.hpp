#pragma once
#include "../StudyApp/AppSettings.hpp"
#include "../vku/VulkanContext.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <string>

namespace vku {
struct FrameDrawer;
class Window;

struct UpdateParams {
  const float deltaTime;
  const Window& win;
  const uint32_t frameInFlightNo;
};

class Study {
 public:
  virtual ~Study() = default;

  inline virtual std::string getName() = 0;
  virtual void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) = 0;
  virtual void onUpdate(const UpdateParams& params) = 0;
  // virtual void onRender(const VulkanContext& vc) = 0;
  virtual void recordCommandBuffer(const VulkanContext& vc, const FrameDrawer& frameDrawer) = 0;
  virtual void onDeinit() = 0;
  // on ImGuiRender, OnUpdate
};
}  // namespace vku