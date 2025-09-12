//
// Created by rain on 8/6/25.
//

#ifndef RAYCAST_H
#define RAYCAST_H

#include <glm/vec3.hpp>

int raycast(const glm::vec3 &from, const glm::vec3 &to, glm::vec3 *result);
float raycast_ground_z(float x, float y, float start_z = 1500.0f);

#endif //RAYCAST_H
