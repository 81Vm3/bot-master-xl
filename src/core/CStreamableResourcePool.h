//
// Created by rain on 8/6/25.
//

#ifndef CSTREAMABLERESOURCEPOOL_H
#define CSTREAMABLERESOURCEPOOL_H

#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include "glm/vec3.hpp"

struct stObject {
    int id;
    int model;
    glm::vec3 position;
    glm::vec3 rotation;
    float drawDistance;
    std::string materialText;
};

struct stMyPickup {
    int id;
    int model;
    glm::vec3 position;
};

struct st3DTextLabel {
    int id;
    glm::vec3 position;
    int attachedPlayer;
    int attachedVehicle;
    std::string text;
    float drawDistance;
    bool testLOS;
};

struct GridCoord {
    int x, y, z;
    bool operator==(const GridCoord &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct GridHash {
    std::size_t operator()(const GridCoord& coord) const {
        std::size_t h1 = std::hash<int>{}(coord.x);
        std::size_t h2 = std::hash<int>{}(coord.y);
        std::size_t h3 = std::hash<int>{}(coord.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);  // 简单组合哈希
    }
};

class CStreamableResourcePool {
public:
    static constexpr size_t MAX_PICKUPS = 4096;
    static constexpr size_t MAX_OBJECTS = 1000;
    static constexpr size_t MAX_LABELS = 1024;

    CStreamableResourcePool();
    ~CStreamableResourcePool();

    void addPickup(const stMyPickup& pickup);
    void addObject(const stObject& object);
    void addLabel(const st3DTextLabel& label);

    void removePickup(int id);
    void removeObject(int id);
    void removeLabel(int id);

    glm::vec3 getPickupPosition(int id);

    void clear();

    std::vector<stMyPickup> getPickupsInRange(const glm::vec3& position, float range) const;
    std::vector<stObject> getObjectsInRange(const glm::vec3& position, float range) const;
    
    // 3D Space-based label search (using spatial hash)
    std::vector<st3DTextLabel> getLabelsInRange(const glm::vec3& position, float range) const;
    
    // Container-based label search (linear search through all labels)
    std::vector<st3DTextLabel> getLabelsInRangeLinear(const glm::vec3& position, float range) const;

    // Label attachment queries
    std::vector<st3DTextLabel> getLabelsAttachedToPlayer(int playerId) const;
    std::vector<st3DTextLabel> getLabelsAttachedToVehicle(int vehicleId) const;

    size_t getPickupCount() const { return pickupCount; }
    size_t getObjectCount() const { return objectCount; }
    size_t getLabelCount() const { return labelCount; }

private:
    std::array<stMyPickup, MAX_PICKUPS> pickups;
    std::array<stObject, MAX_OBJECTS> objects;
    std::array<st3DTextLabel, MAX_LABELS> labels;

    size_t pickupCount = 0;
    size_t objectCount = 0;
    size_t labelCount = 0;

    // ID-to-index mappings for O(1) lookups
    std::unordered_map<int, size_t> pickupIdToIndex;
    std::unordered_map<int, size_t> objectIdToIndex;
    std::unordered_map<int, size_t> labelIdToIndex;

    // 3D Spatial Hash for Labels
    static constexpr float GRID_CELL_SIZE = 2.0f; // 2 units per cell
    std::unordered_map<GridCoord, std::vector<size_t>, GridHash> labelSpatialHash;

    // Attachment hashmaps for Labels
    std::unordered_map<int, std::vector<size_t>> labelsByAttachedPlayer;
    std::unordered_map<int, std::vector<size_t>> labelsByAttachedVehicle;

    // Helper methods for spatial hashing
    GridCoord getGridCoord(const glm::vec3& position) const;
    void addLabelToSpatialHash(size_t labelIndex);
    void removeLabelFromSpatialHash(size_t labelIndex);
    void clearLabelSpatialHash();
    
    // Helper methods for attachment hashmaps
    void addLabelToAttachmentHashmaps(size_t labelIndex);
    void removeLabelFromAttachmentHashmaps(size_t labelIndex);
    void clearLabelAttachmentHashmaps();
};

#endif //CSTREAMABLERESOURCEPOOL_H