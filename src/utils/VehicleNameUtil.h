#ifndef BOTMASTERXL_VEHICLENAMEUTIL_H
#define BOTMASTERXL_VEHICLENAMEUTIL_H

#include <string>
#include <vector>

class VehicleNameUtil {
public:
    static std::string getVehicleName(int modelId);

private:
    static const std::vector<std::string> vehicleNames;
};

#endif //BOTMASTERXL_VEHICLENAMEUTIL_H
