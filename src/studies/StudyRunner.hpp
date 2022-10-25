#pragma once

#include "Study.hpp"

#include <memory>
#include <vector>

namespace vku {
  class StudyRunner {
  private:
    vku::AppSettings appSettings;
    vku::Window window;
    vku::VulkanContext vc;
  public: // TODO: temporary
    std::vector<std::unique_ptr<vku::Study>> studies;

  public:
    StudyRunner();

    // TODO: implement
    void pushStudy(std::unique_ptr<vku::Study> study);
    void popStudy(std::unique_ptr<vku::Study>& study);

    int run();
  };
}