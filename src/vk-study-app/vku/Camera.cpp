#include "Camera.hpp"

#include <imgui.h>

namespace vku {
  const glm::vec3& CameraView::getPosition() const {
    return position;
  }

  glm::mat4 CameraView::getViewFromWorld() const {
    return glm::lookAt(position, position + getDirection(), { 0, 1, 0 });
  }

  glm::vec3 CameraView::getForward() const {
    return getDirection();
  }

  glm::vec3 CameraView::getUp() const {
    return glm::normalize(glm::cross(getRight(), getForward()));
  }

  glm::vec3 CameraView::getRight() const {
    return glm::normalize(glm::cross(getDirection(), { 0, 1, 0 }));
  }

  glm::mat4 Camera::getProjectionFromWorld() const {
    return getProjectionFromView() * getViewFromWorld();
  }

  glm::vec3 FirstPersonCameraView::getDirection() const {
    return {
      // cos/sin x/y/z order taken from: https://learnopengl.com/code_viewer_gh.php?code=includes/learnopengl/camera.h 
      std::cos(yaw) * std::cos(pitch),
      std::sin(pitch),
      std::sin(yaw) * std::cos(pitch),
    };
  }

  glm::mat4 PerspectiveCameraProjection::getProjectionFromView() const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
  }

  glm::mat4 OrthographicCameraProjection::getProjectionFromView() const {
    float left = -size * aspectRatio * 0.5f;
    float right = size * aspectRatio * 0.5f;
    float bottom = -size * 0.5f;
    float top = size * 0.5f;

    return glm::ortho(left, right, bottom, top, nearClip, farClip);
  }

  // -----

  FirstPersonCameraViewOrbitingController::FirstPersonCameraViewOrbitingController(FirstPersonCameraView& cameraView)
    : cameraView(cameraView) { }

  void FirstPersonCameraViewOrbitingController::update(float deltaTime) {
    deltaPhi += speed * deltaTime;
    phi = phi0 + deltaPhi;
    cameraView.position = glm::vec3 {
      std::cos(phi)* std::cos(theta),
      std::sin(theta),
      std::sin(phi)* std::cos(theta),
    } * radius;

    cameraView.yaw = phi + std::numbers::pi_v<float>;
    cameraView.pitch = -theta;

    ImGui::Text("Orbiting Camera Controller");
    ImGui::SliderFloat("Theta", &theta, -std::numbers::pi_v<float> *0.5, std::numbers::pi_v<float> *0.5);
    ImGui::SliderFloat("Phi0", &phi0, -std::numbers::pi_v<float>, std::numbers::pi_v<float>);
    ImGui::SliderFloat("Speed", &speed, -2.f, 2.f);
    ImGui::SliderFloat("Radius", &radius, 0.1f, 10.f);
  }
}