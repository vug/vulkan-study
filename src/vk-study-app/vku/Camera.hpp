#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtx/quaternion.hpp>

#include <numbers>

namespace vku {
  class CameraView {
  private:
    glm::vec3 position{};
  public:
    const glm::vec3& getPosition() const;
    virtual glm::quat getOrientation() const = 0;

    glm::mat4 getViewFromWorld() const;
    glm::vec3 getForward() const;
    glm::vec3 getUp() const;
    glm::vec3 getRight() const;
  };

  class CameraProjection {
  protected:
    float nearClip{ 0.01f };
    float farClip{ 100.0f };
    float aspectRatio{ 1.f };
  public:
    virtual glm::mat4 getProjectionFromView() const = 0;
  };

  class Camera : public CameraView, public CameraProjection {
    glm::mat4 getProjectionFromWorld() const;
  };

  class FirstPersonCameraView : public CameraView {
  public:
    float pitch{};
    float yaw{ std::numbers::pi_v<float> };
    float roll{};
  public:
    virtual glm::quat getOrientation() const final;
  };

  class PerspectiveCameraProjection : public CameraProjection {
  private:
    float fov{50.f};
  public:
    virtual glm::mat4 getProjectionFromView() const final;
  };

  class OrthographicCameraProjection : public CameraProjection {
  private:
    float size{ 5.0f };
  public:
    virtual glm::mat4 getProjectionFromView() const final;
  };

  class FirstPersonPerspectiveCamera : 
    public FirstPersonCameraView, 
    public PerspectiveCameraProjection {};

  class FirstPersonOrthographicCamera :
    public FirstPersonCameraView,
    public OrthographicCameraProjection {};
}