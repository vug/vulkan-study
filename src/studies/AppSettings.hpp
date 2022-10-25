#pragma once
#include <string>

namespace vku {
  struct AppSettings {
    std::string name = "A Vulkan App";
    int32_t width = 800;
    int32_t height = 800;
    bool hasPresentDepth = false;
  };
}