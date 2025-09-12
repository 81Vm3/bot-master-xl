#include "timeutil.h"
#include <sstream>
#include <iomanip>
#include <ctime>

namespace TimeUtil {
    
    std::chrono::system_clock::time_point parseDateTime(const std::string& dateTimeStr) {
        if (dateTimeStr.empty()) {
            return std::chrono::system_clock::time_point{}; // Return epoch time for empty string
        }
        
        std::tm tm = {};
        std::istringstream ss(dateTimeStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        
        if (ss.fail()) {
            // Return epoch time if parsing fails
            return std::chrono::system_clock::time_point{};
        }
        
        // Convert to time_point
        std::time_t time = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(time);
    }
    
    std::string getCurrentTimeString() {
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm* localTime = std::localtime(&time);
        
        std::ostringstream oss;
        oss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
    
    long getMinutesDifference(const std::chrono::system_clock::time_point& from,
                             const std::chrono::system_clock::time_point& to) {
        auto duration = std::chrono::duration_cast<std::chrono::minutes>(to - from);
        return duration.count();
    }
    
    bool isWithinMinutes(const std::string& dateTimeStr, int maxMinutes) {
        auto parsedTime = parseDateTime(dateTimeStr);
        
        // If parsing failed (epoch time returned), consider it not within range
        if (parsedTime == std::chrono::system_clock::time_point{}) {
            return false;
        }
        
        auto now = std::chrono::system_clock::now();
        long minutesDiff = getMinutesDifference(parsedTime, now);
        
        // Return true if the time is within maxMinutes from now (positive means past)
        return (minutesDiff >= 0 && minutesDiff <= maxMinutes);
    }
}