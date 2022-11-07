#include "StudyRunner.hpp"

#include "../vku/ImGuiHelper.hpp"
#include "../vku/SpirvHelper.hpp"
#include "../vku/Window.hpp"
#include "../vku/utils.hpp"

#include <iostream>
#include <chrono> 

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
    
    ImGuiHelper imGuiHelper{ vc, window };

    //---- Main Loop
    while (!window.shouldClose()) {
      window.pollEvents();

      auto time = std::chrono::system_clock::now();
      const vku::FrameDrawer frameDrawer = vc.drawFrameBegin();
      imGuiHelper.Begin();
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
        imGuiHelper.ShowDemoWindow();
      }
      imGuiHelper.End();

      // A final render pass for ImGui draw commands
      const vk::RenderPassBeginInfo renderPassBeginInfo(*vc.renderPass, *frameDrawer.framebuffer, vk::Rect2D{ {0,0}, vc.swapchainExtent }, {});
      frameDrawer.commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
      imGuiHelper.AddDrawCalls(*frameDrawer.commandBuffer);
      frameDrawer.commandBuffer.endRenderPass();

      vc.drawFrameEnd(frameDrawer);
      std::chrono::duration<float> duration = std::chrono::system_clock::now() - time;
      std::cout << "frame duration: " << duration << ", FPS: " << 1.f / duration.count() << '\n';
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