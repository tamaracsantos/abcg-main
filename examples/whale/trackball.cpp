#include "trackball.hpp"

#include <glm/gtc/epsilon.hpp>
#include <limits>

const auto epsilon{std::numeric_limits<float>::epsilon()};

void TrackBall::mouseMove(const glm::ivec2 &position) {
  if (!m_mouseTracking) return;

  const auto msecs{static_cast<float>(m_lastTime.restart()) * 1000.0f};

  // Return if mouse cursor hasn't moved wrt last position
  const auto currentPosition{project(position)};
  if (glm::all(glm::epsilonEqual(m_lastPosition, currentPosition, epsilon)))
    return;

  // Rotation axis
  m_axis = glm::cross(m_lastPosition, currentPosition);

  // Rotation angle
  const auto angle{glm::length(m_axis)};

  m_axis = glm::normalize(m_axis);

  // Compute an angle velocity that will be used as a constant rotation angle
  // when the mouse is not being tracked.
  m_velocity = angle / (msecs + epsilon);
  m_velocity = glm::clamp(m_velocity, 0.0f, m_maxVelocity);

  // Concatenate the rotation: R_old = R_new * R_old
  m_rotation = glm::rotate(glm::mat4(1.0f), angle, m_axis) * m_rotation;

  m_lastPosition = currentPosition;
}

void TrackBall::mousePress(const glm::ivec2 &position) {
  m_rotation = getRotation();
  m_mouseTracking = true;

  m_lastTime.restart();

  m_lastPosition = project(position);

  m_velocity = 0.0f;
}

void TrackBall::mouseRelease(const glm::ivec2 &position) {
  mouseMove(position);
  m_mouseTracking = false;
}

void TrackBall::resizeViewport(int width, int height) {
  m_viewportWidth = static_cast<float>(width);
  m_viewportHeight = static_cast<float>(height);
}

glm::mat4 TrackBall::getRotation() {
  if (m_mouseTracking) return m_rotation;

  // If not tracking, rotate by velocity. This will simulate
  // an inertia-free rotation.
  const auto angle{m_velocity * static_cast<float>(m_lastTime.elapsed()) *
                   1000.0f};

  return glm::rotate(glm::mat4(1.0f), angle, m_axis) * m_rotation;
}

glm::vec3 TrackBall::project(const glm::vec2 &position) const {
  // Convert from window coordinates to NDC
  auto v{glm::vec3(2.0f * position.x / m_viewportWidth - 1.0f,
                   1.0f - 2.0f * position.y / m_viewportHeight, 0.0f)};

  // Project to centered unit hemisphere
  const auto squaredLength{glm::length2(v)};
  if (squaredLength >= 1.0f) {
    // Outside sphere
    v = glm::normalize(v);
  } else {
    // Inside sphere
    v.z = std::sqrt(1.0f - squaredLength);
  }

  return v;
}