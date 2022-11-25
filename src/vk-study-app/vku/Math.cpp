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
}  // namespace vku