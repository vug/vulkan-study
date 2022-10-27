#pragma once

#include "../StudyApp/Study.hpp"

#include <memory>

struct Vertex {
  float pos[3];
  float col[3];
};

struct Buffer {
  vk::raii::Buffer buffer = nullptr;
  vk::raii::DeviceMemory memory = nullptr;
};

class VerticesStudy : public vku::Study {
private:
  Buffer vertices;
  Buffer indices;
  uint32_t indexCount;
  std::unique_ptr<vk::raii::Pipeline> pipeline;

public:
  virtual ~VerticesStudy() = default;

  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};