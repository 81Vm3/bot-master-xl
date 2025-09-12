//
// Created by rain on 8/6/25.
//

#include "Raycast.h"

#include <LinearMath/btVector3.h>
#include "spdlog/spdlog.h"

#include "CApp.h"
#include "vendor/ColAndreas/DynamicWorld.h"

int raycast(const glm::vec3 &from, const glm::vec3 &to, glm::vec3 *result) {

    btVector3 Start = btVector3(btScalar(from.x + 0.00001), btScalar(from.y + 0.00001), btScalar(from.z + 0.00001));
    btVector3 End = btVector3(btScalar(to.x), btScalar(to.y), btScalar(to.z));
    btVector3 Result;
    int32_t iModel = 0;

    auto colAndreas = CApp::getInstance()->getColAndreas();
    if (!colAndreas) {
        spdlog::error("ColAndreas instance is null!");
        return 0;
    }
    if (colAndreas->performRayTest(Start, End, Result, iModel)) {
        result->x = Result.getX();
        result->y = Result.getY();
        result->z = Result.getZ();
        return iModel;
    }
    return 0;
}

float raycast_ground_z(float x, float y, float start_z) {
    glm::vec3 from = {x, y, start_z};
    glm::vec3 to = {x, y, -50.0f}; // Cast down to well below ground level
    glm::vec3 hitPoint;
    
    if (raycast(from, to, &hitPoint)) {
        return hitPoint.z;
    }
    
    // If no hit found, return a default ground level
    return 0.0f;
}
