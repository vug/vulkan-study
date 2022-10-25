#include "StudyRunner.hpp"

#include "AppSettings.hpp"
#include "SpirvHelper.hpp"
#include "VulkanContext.hpp"
#include "Window.hpp"

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

    studies.front()->onInit(vc, appSettings);

    //---- Main Loop
    while (!window.shouldClose()) {
      window.pollEvents();

      auto func = [&](const vk::raii::CommandBuffer& c, const vk::RenderPassBeginInfo& r) {studies.front()->recordCommandBuffer(c, r); };
      vc.drawFrame(func);
    }

    // END
    vc.device.waitIdle();

    studies.front()->onDeinit();

    vku::spirv::finalize();
    // Need to be destroyed explicitly becomes raii instance does not own it apparently.

    std::cout << "Bye, Vulkan!\n";
    return 0;
  }
}