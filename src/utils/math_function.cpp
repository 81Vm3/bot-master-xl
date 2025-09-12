//
// Created by rain on 8/7/25.
//

#include "math_function.h"

#include <cstdlib>

float RandomFloat(float min, float max) {
    float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    float range = max - min;
    return (random * range) + min;
}
