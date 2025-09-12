//
// Created by rain on 8/5/25.
//
#include "CPathFinder.h"

#include <queue>
#include <glm/vec2.hpp>
#include <glm/geometric.hpp>

#include "CApp.h"
#include "Raycast.h"

std::vector<glm::vec3> CPathFinder::findPath(glm::vec3 from, glm::vec3 to) {
    if (glm::distance(from, to) > 150.0) return {};

    std::vector<glm::vec3> result;

    glm::vec2 from2D(from.x, from.y);
    glm::vec2 to2D(to.x, to.y);
    glm::vec2 center2D = (from2D + to2D) * 0.5f;

    float radius = glm::distance(from2D, to2D);
    float spacing = 1;

    for (float x = -radius; x <= radius; x += spacing) {
        for (float y = -radius; y <= radius; y += spacing) {
            glm::vec2 offset(x, y);
            if (glm::length(offset) > radius)
                continue;

            glm::vec2 point2D = center2D + offset;
            glm::vec3 rayStart(point2D.x, point2D.y, 1000.0f);
            glm::vec3 rayEnd(point2D.x, point2D.y, -1000.0f);

            glm::vec3 hitPoint;
            if (raycast(rayStart, rayEnd, &hitPoint)) {
                result.push_back(hitPoint);
            }
        }
    }

    result.push_back(from); // index: n - 2
    result.push_back(to);   // index: n - 1

    int n = result.size();
    glm::vec3 tmp;
    std::vector<std::vector<bool>> edges(n, std::vector<bool>(n, false));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            if (abs(result[i].z - result[j].z) < 1.08f) {
                edges[i][j] = edges[j][i] = true;
            }
        }
    }

    std::vector<bool> vis(n, false);
    std::vector<int> prev(n, -1);
    std::queue<int> q;

    vis[n - 2] = true; // from
    q.push(n - 2);

    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v = 0; v < n; ++v) {
            if (edges[u][v] && !vis[v]) {
                vis[v] = true;
                prev[v] = u;
                q.push(v);
            }
        }
    }

    // 回溯路径
    std::vector<glm::vec3> path;
    for (int at = n - 1; at != -1; at = prev[at]) {
        path.push_back(result[at]);
    }
    std::reverse(path.begin(), path.end());
    return path;
}


void CPathFinder::findPathAsync(glm::vec3 from, glm::vec3 to, std::function<void(std::vector<glm::vec3>)> callback) {
}
