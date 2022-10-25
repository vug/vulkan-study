#pragma once

#include "Study.hpp"

#include <list>
#include <memory>

namespace vku {
  class StudyRunner {
  private:
    vku::AppSettings appSettings;
    vku::Window window;
    vku::VulkanContext vc;
    std::list<std::unique_ptr<vku::Study>> studies;
  public:

  public:
    StudyRunner();

    std::unique_ptr<vku::Study>& pushStudy(std::unique_ptr<vku::Study> study);
    void popStudy(const std::unique_ptr<vku::Study>& study);

    int run();
  };
}