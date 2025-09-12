//
// Created by rain on 8/6/25.
//

#include "CStreamableResourcePool.h"
#include <cmath>
#include <algorithm>

CStreamableResourcePool::CStreamableResourcePool() = default;

CStreamableResourcePool::~CStreamableResourcePool() = default;

void CStreamableResourcePool::addPickup(const stMyPickup& pickup) {
    if (pickupCount >= MAX_PICKUPS) return;
    
    pickups[pickupCount] = pickup;
    pickupIdToIndex[pickup.id] = pickupCount;
    pickupCount++;
}

void CStreamableResourcePool::addObject(const stObject& object) {
    if (objectCount >= MAX_OBJECTS) return;
    
    objects[objectCount] = object;
    objectIdToIndex[object.id] = objectCount;
    objectCount++;
}

void CStreamableResourcePool::addLabel(const st3DTextLabel& label) {
    if (labelCount >= MAX_LABELS) return;
    
    labels[labelCount] = label;
    labelIdToIndex[label.id] = labelCount;
    addLabelToSpatialHash(labelCount);
    addLabelToAttachmentHashmaps(labelCount);
    labelCount++;
}

void CStreamableResourcePool::removePickup(int id) {
    auto it = pickupIdToIndex.find(id);
    if (it == pickupIdToIndex.end()) return; // ID not found
    
    size_t index = it->second;
    
    // Remove from ID mapping
    pickupIdToIndex.erase(it);
    
    // If not the last element, swap with last element
    if (index < pickupCount - 1) {
        pickups[index] = pickups[pickupCount - 1];
        // Update the ID mapping for the moved element
        pickupIdToIndex[pickups[index].id] = index;
    }
    pickupCount--;
}

void CStreamableResourcePool::removeObject(int id) {
    auto it = objectIdToIndex.find(id);
    if (it == objectIdToIndex.end()) return; // ID not found
    
    size_t index = it->second;
    
    // Remove from ID mapping
    objectIdToIndex.erase(it);
    
    // If not the last element, swap with last element
    if (index < objectCount - 1) {
        objects[index] = objects[objectCount - 1];
        // Update the ID mapping for the moved element
        objectIdToIndex[objects[index].id] = index;
    }
    objectCount--;
}

void CStreamableResourcePool::removeLabel(int id) {
    auto it = labelIdToIndex.find(id);
    if (it == labelIdToIndex.end()) return; // ID not found
    
    size_t index = it->second;
    
    // Remove from spatial hash and attachment hashmaps
    removeLabelFromSpatialHash(index);
    removeLabelFromAttachmentHashmaps(index);
    
    // Remove from ID mapping
    labelIdToIndex.erase(it);
    
    // If not the last element, swap with last element
    if (index < labelCount - 1) {
        // Remove the last element from spatial structures first
        removeLabelFromSpatialHash(labelCount - 1);
        removeLabelFromAttachmentHashmaps(labelCount - 1);
        
        // Move the last label to fill the gap
        labels[index] = labels[labelCount - 1];
        
        // Update ID mapping for the moved element
        labelIdToIndex[labels[index].id] = index;
        
        // Re-add the moved label to spatial structures at new index
        addLabelToSpatialHash(index);
        addLabelToAttachmentHashmaps(index);
    }
    labelCount--;
}

glm::vec3 CStreamableResourcePool::getPickupPosition(int id) {
    auto it = pickupIdToIndex.find(id);
    if (it != pickupIdToIndex.end()) {
        return pickups[it->second].position;
    }
    return glm::vec3(0.0f); // Default position if not found
}

void CStreamableResourcePool::clear() {
    pickupCount = 0;
    objectCount = 0;
    labelCount = 0;
    
    // Clear ID-to-index mappings
    pickupIdToIndex.clear();
    objectIdToIndex.clear();
    labelIdToIndex.clear();
    
    clearLabelSpatialHash();
    clearLabelAttachmentHashmaps();
}

std::vector<stMyPickup> CStreamableResourcePool::getPickupsInRange(const glm::vec3& position, float range) const {
    std::vector<stMyPickup> result;
    float rangeSq = range * range;
    
    for (size_t i = 0; i < pickupCount; i++) {
        glm::vec3 diff = pickups[i].position - position;
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= rangeSq) {
            result.push_back(pickups[i]);
        }
    }
    
    return result;
}

std::vector<stObject> CStreamableResourcePool::getObjectsInRange(const glm::vec3& position, float range) const {
    std::vector<stObject> result;
    float rangeSq = range * range;
    
    for (size_t i = 0; i < objectCount; i++) {
        glm::vec3 diff = objects[i].position - position;
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= rangeSq) {
            result.push_back(objects[i]);
        }
    }
    
    return result;
}

// 3D Space-based label search using spatial hash (optimized for many labels)
std::vector<st3DTextLabel> CStreamableResourcePool::getLabelsInRange(const glm::vec3& position, float range) const {
    std::vector<st3DTextLabel> result;
    float rangeSq = range * range;
    
    // Calculate grid bounds for the search area
    GridCoord minGrid = getGridCoord(position - glm::vec3(range));
    GridCoord maxGrid = getGridCoord(position + glm::vec3(range));
    
    // Search through relevant grid cells
    for (int x = minGrid.x; x <= maxGrid.x; x++) {
        for (int y = minGrid.y; y <= maxGrid.y; y++) {
            for (int z = minGrid.z; z <= maxGrid.z; z++) {
                GridCoord coord = {x, y, z};
                auto it = labelSpatialHash.find(coord);
                if (it != labelSpatialHash.end()) {
                    // Check each label in this grid cell
                    for (size_t labelIndex : it->second) {
                        if (labelIndex < labelCount) {
                            const auto& label = labels[labelIndex];
                            glm::vec3 diff = label.position - position;
                            if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= rangeSq) {
                                result.push_back(label);
                            }
                        }
                    }
                }
            }
        }
    }
    
    return result;
}

// Container-based label search (linear search through all labels)
std::vector<st3DTextLabel> CStreamableResourcePool::getLabelsInRangeLinear(const glm::vec3& position, float range) const {
    std::vector<st3DTextLabel> result;
    float rangeSq = range * range;
    
    // Linear search through all labels in the container
    for (size_t i = 0; i < labelCount; i++) {
        const auto& label = labels[i];
        glm::vec3 diff = label.position - position;
        if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z <= rangeSq) {
            result.push_back(label);
        }
    }
    
    return result;
}

// Spatial Hash Helper Functions
GridCoord CStreamableResourcePool::getGridCoord(const glm::vec3& position) const {
    return {
        static_cast<int>(std::floor(position.x / GRID_CELL_SIZE)),
        static_cast<int>(std::floor(position.y / GRID_CELL_SIZE)),
        static_cast<int>(std::floor(position.z / GRID_CELL_SIZE))
    };
}

void CStreamableResourcePool::addLabelToSpatialHash(size_t labelIndex) {
    if (labelIndex >= labelCount) return;
    
    GridCoord coord = getGridCoord(labels[labelIndex].position);
    labelSpatialHash[coord].push_back(labelIndex);
}

void CStreamableResourcePool::removeLabelFromSpatialHash(size_t labelIndex) {
    if (labelIndex >= labelCount) return;
    
    GridCoord coord = getGridCoord(labels[labelIndex].position);
    auto it = labelSpatialHash.find(coord);
    if (it != labelSpatialHash.end()) {
        auto& indices = it->second;
        indices.erase(std::remove(indices.begin(), indices.end(), labelIndex), indices.end());
        
        // Remove empty grid cells
        if (indices.empty()) {
            labelSpatialHash.erase(it);
        }
    }
}

void CStreamableResourcePool::clearLabelSpatialHash() {
    labelSpatialHash.clear();
}

// Label Attachment Query Methods
std::vector<st3DTextLabel> CStreamableResourcePool::getLabelsAttachedToPlayer(int playerId) const {
    std::vector<st3DTextLabel> result;
    
    auto it = labelsByAttachedPlayer.find(playerId);
    if (it != labelsByAttachedPlayer.end()) {
        for (size_t labelIndex : it->second) {
            if (labelIndex < labelCount) {
                result.push_back(labels[labelIndex]);
            }
        }
    }
    
    return result;
}

std::vector<st3DTextLabel> CStreamableResourcePool::getLabelsAttachedToVehicle(int vehicleId) const {
    std::vector<st3DTextLabel> result;
    
    auto it = labelsByAttachedVehicle.find(vehicleId);
    if (it != labelsByAttachedVehicle.end()) {
        for (size_t labelIndex : it->second) {
            if (labelIndex < labelCount) {
                result.push_back(labels[labelIndex]);
            }
        }
    }
    
    return result;
}

// Attachment Hashmap Helper Functions
void CStreamableResourcePool::addLabelToAttachmentHashmaps(size_t labelIndex) {
    if (labelIndex >= labelCount) return;
    
    const auto& label = labels[labelIndex];
    
    // Add to player attachment hashmap if attached to a player
    if (label.attachedPlayer != -1) {
        labelsByAttachedPlayer[label.attachedPlayer].push_back(labelIndex);
    }
    
    // Add to vehicle attachment hashmap if attached to a vehicle
    if (label.attachedVehicle != -1) {
        labelsByAttachedVehicle[label.attachedVehicle].push_back(labelIndex);
    }
}

void CStreamableResourcePool::removeLabelFromAttachmentHashmaps(size_t labelIndex) {
    if (labelIndex >= labelCount) return;
    
    const auto& label = labels[labelIndex];
    
    // Remove from player attachment hashmap
    if (label.attachedPlayer != -1) {
        auto it = labelsByAttachedPlayer.find(label.attachedPlayer);
        if (it != labelsByAttachedPlayer.end()) {
            auto& indices = it->second;
            indices.erase(std::remove(indices.begin(), indices.end(), labelIndex), indices.end());
            
            // Remove empty entries
            if (indices.empty()) {
                labelsByAttachedPlayer.erase(it);
            }
        }
    }
    
    // Remove from vehicle attachment hashmap
    if (label.attachedVehicle != -1) {
        auto it = labelsByAttachedVehicle.find(label.attachedVehicle);
        if (it != labelsByAttachedVehicle.end()) {
            auto& indices = it->second;
            indices.erase(std::remove(indices.begin(), indices.end(), labelIndex), indices.end());
            
            // Remove empty entries
            if (indices.empty()) {
                labelsByAttachedVehicle.erase(it);
            }
        }
    }
}

void CStreamableResourcePool::clearLabelAttachmentHashmaps() {
    labelsByAttachedPlayer.clear();
    labelsByAttachedVehicle.clear();
}