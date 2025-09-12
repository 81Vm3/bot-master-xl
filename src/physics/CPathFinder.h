//
// Created by rain on 8/5/25.
//

#ifndef CPATHFINDER_H
#define CPATHFINDER_H
#include <vector>

#include "glm/vec3.hpp"
#include "glm/detail/type_vec4.hpp"

class CPathFinder {
public:
    std::vector<glm::vec3> findPath(glm::vec3 from, glm::vec3 to);
    void findPathAsync(glm::vec3 from, glm::vec3 to, std::function<void(std::vector<glm::vec3>)> callback);
private:

};




#endif //CPATHFINDER_H
