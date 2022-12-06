#include "Math.hpp"

namespace vku {
Transform::Transform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& sca)
    : position{pos}, rotation{rot}, scale{sca} {}

Transform::Transform(const glm::vec3& pos, const glm::vec3 axis, const float angle, const glm::vec3& sca)
    : position{pos}, rotation{glm::angleAxis(angle, axis)}, scale{sca} {}

glm::mat4 Transform::getTranslateMatrix() const {
  return glm::translate(glm::mat4(1), position);
}

glm::mat4 Transform::getRotationMatrix() const {
  return glm::toMat4(rotation);  // mat4_cast
}

glm::mat4 Transform::getScaleMatrix() const {
  return glm::scale(glm::mat4(1), scale);
}

glm::mat4 Transform::getTransform() const {
  return getTranslateMatrix() * getRotationMatrix() * getScaleMatrix();
}

TransformGPU Transform::toGPULayout() const {
  return TransformGPU{
      .position = glm::vec4(position, 1.f),
      .rotation = rotation,
      .scale = glm::vec4(scale, 1.f),
  };
}

glm::quat rotateTowards(glm::quat q1, glm::quat q2, float maxAngle) {
  if (maxAngle < 0.00001f)
    return q1;

  float cosTheta = glm::dot(q1, q2);

  if (cosTheta > 0.99999f)
    return q2;

  // take shorter path on the sphere
  if (cosTheta < 0) {
    q1 *= -1.f;
    cosTheta *= -1.f;
  }
  float angle = glm::acos(cosTheta);

  if (angle < maxAngle)
    return q2;

  // because we make sure shorter path is taken above, we can use mix instead of slerp (which ensures shorter path)
  const float m = maxAngle / angle;
  return glm::mix(q1, q2, m);
}
}  // namespace vku