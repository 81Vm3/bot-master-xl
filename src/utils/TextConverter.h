//
// Created by rain on 8/2/25.
//

#ifndef TEXTCONVERTER_H
#define TEXTCONVERTER_H

#include <string>
#include <vector>

class TextConverter {
public:
    /**
     * Convert GB2312 encoded string to UTF-8
     * @param gb2312_str Input string in GB2312 encoding
     * @return String converted to UTF-8 encoding
     */
    static std::string gb2312ToUtf8(const std::string& gb2312_str);
    
    /**
     * Convert GB2312 encoded char buffer to UTF-8
     * @param gb2312_buffer Input buffer in GB2312 encoding
     * @param length Length of input buffer
     * @return String converted to UTF-8 encoding
     */
    static std::string gb2312ToUtf8(const char* gb2312_buffer, size_t length);
    
    /**
     * Convert UTF-8 encoded string to GB2312
     * @param utf8_str Input string in UTF-8 encoding
     * @return String converted to GB2312 encoding
     */
    static std::string utf8ToGb2312(const std::string& utf8_str);
    
    /**
     * Check if a string is valid UTF-8
     * @param str String to check
     * @return true if valid UTF-8, false otherwise
     */
    static bool isValidUtf8(const std::string& str);
    
    /**
     * Auto-detect encoding and convert to UTF-8 if needed
     * @param input Input string that might be GB2312 or UTF-8
     * @return String guaranteed to be UTF-8
     */
    static std::string ensureUtf8(const std::string& input);

private:
    /**
     * Convert single GB2312 character to UTF-8
     * @param gb2312_char GB2312 character code
     * @return UTF-8 encoded bytes
     */
    static std::vector<uint8_t> gb2312CharToUtf8(uint16_t gb2312_char);
    
    /**
     * Convert UTF-8 character to GB2312
     * @param utf8_bytes UTF-8 encoded bytes
     * @param bytes_used Number of bytes consumed
     * @return GB2312 character code (0 if conversion failed)
     */
    static uint16_t utf8CharToGb2312(const uint8_t* utf8_bytes, int& bytes_used);
    
    /**
     * Get the length of a UTF-8 character from its first byte
     * @param first_byte First byte of UTF-8 character
     * @return Length in bytes (1-4), or 0 if invalid
     */
    static int getUtf8CharLength(uint8_t first_byte);
};

#endif //TEXTCONVERTER_H