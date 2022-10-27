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
  std::unique_ptr<vk::raii::Pipeline> pipeline;
  std::unique_ptr<vk::raii::CommandBuffer> copyCmdBuf;
  std::vector<Vertex> vertexBuffer;
  std::vector<uint32_t> indexBuffer;
  

public:
  virtual ~VerticesStudy() = default;

  void onInit(const vku::AppSettings appSettings, const vku::VulkanContext& vc) final;
  void recordCommandBuffer(const vku::VulkanContext& vc, const vku::FrameDrawer& frameDrawer) final;
  void onDeinit() final;
};