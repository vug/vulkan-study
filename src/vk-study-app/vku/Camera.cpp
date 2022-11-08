#include "Camera.hpp"

namespace vku {
  const glm::vec3& CameraView::getPosition() const {
    return position;
  }

  glm::mat4 CameraView::getViewFromWorld() const {
    return glm::translate(glm::mat4(1.0f), getPosition()) * glm::toMat4(getOrientation());
  }

  glm::vec3 CameraView::getForward() const {
    return glm::rotate(getOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
  }

  glm::vec3 CameraView::getUp() const {
    return glm::rotate(getOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
  }

  glm::vec3 CameraView::getRight() const {
    return glm::rotate(getOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
  }

  glm::mat4 Camera::getProjectionFromWorld() const {
    return getProjectionFromView() * getViewFromWorld();
  }

  glm::quat FirstPersonCameraView::getOrientation() const {
    return glm::quat(glm::vec3(pitch, yaw, roll));
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