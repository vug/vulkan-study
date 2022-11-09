#include "Camera.hpp"

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
}