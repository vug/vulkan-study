#include "StudyRunner.hpp"

#include "AppSettings.hpp"
#include "SpirvHelper.hpp"
#include "VulkanContext.hpp"
#include "Window.hpp"
#include "utils.hpp"

#include <iostream>

namespace vku {
  StudyRunner::StudyRunner() :
    appSettings({
      .name = "vk-bootstrap variation study",
      .hasPresentDepth = false,
    }),
    window(),
    vc(window, appSettings)
  {}

  int StudyRunner::run() {
    std::cout << "Hello, Vulkan!\n";

    vku::spirv::init();
    for (auto& study : studies)
      study->onInit();

    //---- Main Loop
    while (!window.shouldClose()) {
      window.pollEvents();

      auto func = [&](const vk::raii::CommandBuffer& cmdBuf, const vk::raii::Framebuffer& framebuffer) {
        for (auto& study : studies)
          study->recordCommandBuffer(cmdBuf, framebuffer);
      };
      vc.drawFrame(func);
    }

    // END
    vc.device.waitIdle();

    for (auto& study : studies)
      studies.back()->onDeinit();
    vku::spirv::finalize();
    // Need to be destroyed explicitly becomes raii instance does not own it apparently.

    std::cout << "Bye, Vulkan!\n";
    return 0;
  }

  std::unique_ptr<vku::Study>& StudyRunner::pushStudy(std::unique_ptr<vku::Study> study) {
    studies.push_back(std::move(study));
    return studies.back();
  }

  void StudyRunner::popStudy(const std::unique_ptr<vku::Study>& study) {
    studies.remove(study);
  }

}