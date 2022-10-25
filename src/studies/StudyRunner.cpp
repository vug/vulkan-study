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
    for (auto& study : studies)
      study->onInit(vc, appSettings);

    //---- Main Loop
    while (!window.shouldClose()) {
      window.pollEvents();

      auto func = [&](const vk::raii::CommandBuffer& c) 
        // TODO: For now each study adds commands into the same command buffer under default renderass
        // Later let each Study to add commands into the same command buffer but using different RenderPasses.
        // I guess layers shouldn't clear and successive RenderPasses need to be synchronized via pipeline barriers
        // Also it could be the case that first Study and the rest might be given different RenderPasses
        // first one clears and does not care about load whereas the rest don't clear and "have loadOp"
        { for (auto& study : studies) study->recordCommandBuffer(c); };
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