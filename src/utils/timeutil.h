#pragma once

#include <chrono>
#include <string>

namespace TimeUtil {
    
    /**
     * Parse datetime string in format "YYYY-MM-DD HH:MM:SS" to chrono time_point
     * @param dateTimeStr The datetime string to parse
     * @return chrono time_point, or epoch time if parsing fails
     */
    std::chrono::system_clock::time_point parseDateTime(const std::string& dateTimeStr);
    
    /**
     * Get current time as formatted string "YYYY-MM-DD HH:MM:SS"
     * @return Formatted current time string
     */
    std::string getCurrentTimeString();
    
    /**
     * Calculate minutes difference between two time points
     * @param from Start time point
     * @param to End time point  
     * @return Minutes difference (positive if 'to' is after 'from')
     */
    long getMinutesDifference(const std::chrono::system_clock::time_point& from,
                             const std::chrono::system_clock::time_point& to);
    
    /**
     * Check if a datetime string represents a time within the specified minutes from now
     * @param dateTimeStr The datetime string to check
     * @param maxMinutes Maximum minutes ago to consider as recent
     * @return true if the time is within maxMinutes from now
     */
    bool isWithinMinutes(const std::string& dateTimeStr, int maxMinutes);
}