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
      study->onInit(appSettings, vc);

    //---- Main Loop
    while (!window.shouldClose()) {
      window.pollEvents();

      const vku::FrameDrawer frameDrawer = vc.drawFrameBegin();
      for (auto& study : studies) {
        study->recordCommandBuffer(vc, frameDrawer);
        // TODO: might need to add some synchronization here. If layer order does not look correct uncomment below line.
        //vku::setImageLayout(frameDrawer.commandBuffer, frameDrawer.image, vc.swapchainColorFormat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal);
      }
      vc.drawFrameEnd(frameDrawer);
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