#pragma once
#include "game.h"
#include "physics.h"
#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL

namespace collision {
bool IsColliding(const cCollider &c1, const cCollider &c2, glm::dvec3 &pos, glm::dvec3 &norm, double &depth);
}
