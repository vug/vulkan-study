#include "StudyRunner.hpp"

#include "../vku/SpirvHelper.hpp"
#include "../vku/Window.hpp"
#include "../vku/utils.hpp"

#include <iostream>

namespace vku {
  StudyRunner::StudyRunner() :
    appSettings({
      .name = "A Vulkan Study Application",
      .hasPresentDepth = true,
    }),
    window(appSettings),
    vc(window, appSettings)
  {}

  int StudyRunner::run() {
    std::cout << "Hello, Vulkan!\n";

    vku::spirv::init();
    for (auto& study : studies) {
      std::cout << std::format("Loading Study: '{}'\n", study->getName());;
      study->onInit(appSettings, vc);
    }
      
    //---- Main Loop
    while (!window.shouldClose()) {
      window.pollEvents();

      const vku::FrameDrawer frameDrawer = vc.drawFrameBegin();
      for (auto& study : studies) {
        study->recordCommandBuffer(vc, frameDrawer);
        // TODO: might need to add some synchronization here. If layer order does not look correct uncomment below line.
        //vku::setImageLayout(frameDrawer.commandBuffer, frameDrawer.image, vc.swapchainColorFormat, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal);
        // or
        //vkCmdPipelineBarrier(
        //  *frameDrawer.commandBuffer, 
        //  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        //  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
        //  {},
        //  0, nullptr, 0, nullptr, 0, nullptr);
      }
      vc.drawFrameEnd(frameDrawer);
    }

    // END
    vc.device.waitIdle();

    for (auto& study : studies)
      study->onDeinit();
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