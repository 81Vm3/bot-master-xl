//
// Created by rain on 8/2/25.
//

#include "TextConverter.h"
#include <iconv.h>
#include <errno.h>
#include <cstring>
#include <spdlog/spdlog.h>

std::string TextConverter::gb2312ToUtf8(const std::string& gb2312_str) {
    return gb2312ToUtf8(gb2312_str.c_str(), gb2312_str.length());
}

std::string TextConverter::gb2312ToUtf8(const char* gb2312_buffer, size_t length) {
    if (!gb2312_buffer || length == 0) {
        return "";
    }
    
    // Open iconv descriptor for GBK to UTF-8 conversion
    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == (iconv_t)-1) {
        spdlog::error("Failed to open iconv descriptor for GBK->UTF-8: {}", strerror(errno));
        return std::string(gb2312_buffer, length); // Return original if conversion fails
    }
    
    // Prepare input and output buffers
    size_t in_bytes_left = length;
    size_t out_bytes_left = length * 4; // UTF-8 can be up to 4 bytes per character
    
    const char* in_buf = const_cast<char*>(gb2312_buffer);
    std::vector<char> out_buffer(out_bytes_left);
    char* out_buf = out_buffer.data();
    char* out_buf_start = out_buf;
    
    // Perform conversion
    size_t result = iconv(cd, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
    
    // Close iconv descriptor
    iconv_close(cd);
    
    if (result == (size_t)-1) {
        spdlog::warn("iconv conversion failed: {}", strerror(errno));
        return std::string(gb2312_buffer, length); // Return original if conversion fails
    }
    
    // Calculate converted length
    size_t converted_length = out_buf - out_buf_start;
    return std::string(out_buf_start, converted_length);
}

std::string TextConverter::utf8ToGb2312(const std::string& utf8_str) {
    if (utf8_str.empty()) {
        return "";
    }
    
    // Force conversion approach - process character by character
    std::string result;
    result.reserve(utf8_str.length() * 2);
    
    // Open iconv descriptor for UTF-8 to GBK conversion
    iconv_t cd = iconv_open("GBK//IGNORE", "UTF-8");
    if (cd == (iconv_t)-1) {
        // If iconv fails to open, fall back to ASCII-only filtering
        spdlog::warn("Failed to open iconv descriptor, using ASCII fallback");
        for (unsigned char c : utf8_str) {
            if (c < 128) { // ASCII characters
                result += static_cast<char>(c);
            } else {
                result += '?'; // Replace non-ASCII with placeholder
            }
        }
        return result;
    }
    
    // Process the string in chunks, handling errors gracefully
    size_t pos = 0;
    while (pos < utf8_str.length()) {
        // Try to find the next valid UTF-8 character boundary
        size_t char_start = pos;
        size_t char_len = 1;
        
        // Determine UTF-8 character length
        unsigned char first_byte = static_cast<unsigned char>(utf8_str[pos]);
        if (first_byte < 0x80) {
            char_len = 1; // ASCII
        } else if ((first_byte & 0xE0) == 0xC0) {
            char_len = 2; // 2-byte UTF-8
        } else if ((first_byte & 0xF0) == 0xE0) {
            char_len = 3; // 3-byte UTF-8
        } else if ((first_byte & 0xF8) == 0xF0) {
            char_len = 4; // 4-byte UTF-8
        } else {
            // Invalid UTF-8 start byte, skip it
            pos++;
            continue;
        }
        
        // Make sure we don't go beyond string bounds
        if (char_start + char_len > utf8_str.length()) {
            // Incomplete character at end, skip remaining bytes
            break;
        }
        
        // Extract the character
        std::string utf8_char = utf8_str.substr(char_start, char_len);
        
        // Try to convert this single character
        size_t in_bytes_left = utf8_char.length();
        size_t out_bytes_left = 6; // Generous buffer for single character
        
        std::string input_copy = utf8_char;
        const char* in_buf = const_cast<char*>(input_copy.c_str());
        char out_buffer[6] = {0};
        char* out_buf = out_buffer;
        char* out_buf_start = out_buf;
        
        // Reset iconv state
        iconv(cd, nullptr, nullptr, nullptr, nullptr);
        
        size_t conv_result = iconv(cd, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
        
        if (conv_result != (size_t)-1 && out_buf > out_buf_start) {
            // Successful conversion
            result.append(out_buf_start, out_buf - out_buf_start);
        } else {
            // Conversion failed for this character
            if (first_byte < 128) {
                // ASCII character, keep as-is
                result += static_cast<char>(first_byte);
            } else {
                // Non-ASCII character that can't be converted, replace with placeholder
                result += '?';
            }
        }
        
        pos = char_start + char_len;
    }
    
    iconv_close(cd);
    
    // Final safety check - ensure result contains only printable characters
    std::string safe_result;
    safe_result.reserve(result.length());
    
    for (unsigned char c : result) {
        if (c >= 32 && c <= 126) { // Printable ASCII
            safe_result += static_cast<char>(c);
        } else if (c >= 128) { // Extended characters (GB2312)
            safe_result += static_cast<char>(c);
        } else if (c == '\n' || c == '\r' || c == '\t') { // Common whitespace
            safe_result += static_cast<char>(c);
        } else {
            // Skip control characters that might break JSON
            safe_result += ' ';
        }
    }
    
    return safe_result;
}

bool TextConverter::isValidUtf8(const std::string& str) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(str.c_str());
    size_t len = str.length();
    
    for (size_t i = 0; i < len; ) {
        int char_len = getUtf8CharLength(bytes[i]);
        
        if (char_len == 0) {
            return false; // Invalid first byte
        }
        
        if (i + char_len > len) {
            return false; // Not enough bytes for character
        }
        
        // Check continuation bytes
        for (int j = 1; j < char_len; j++) {
            if ((bytes[i + j] & 0xC0) != 0x80) {
                return false; // Invalid continuation byte
            }
        }
        
        i += char_len;
    }
    
    return true;
}

std::string TextConverter::ensureUtf8(const std::string& input) {
    if (isValidUtf8(input)) {
        return input; // Already UTF-8
    }
    
    // Try to convert from GBK
    std::string converted = gb2312ToUtf8(input);
    
    // Check if conversion was successful
    if (isValidUtf8(converted)) {
        return converted;
    }
    
    // If all else fails, return original (might be already UTF-8 with some encoding issues)
    spdlog::warn("Could not determine encoding for string, returning as-is");
    spdlog::info(input);
    return input;
}

std::vector<uint8_t> TextConverter::gb2312CharToUtf8(uint16_t gb2312_char) {
    // This is a simplified implementation
    // For a complete implementation, you would need the full GB2312 to Unicode mapping table
    std::vector<uint8_t> result;
    
    // Convert GBK character using iconv
    char gb_bytes[3] = {0};
    if (gb2312_char > 0xFF) {
        gb_bytes[0] = (gb2312_char >> 8) & 0xFF;
        gb_bytes[1] = gb2312_char & 0xFF;
    } else {
        gb_bytes[0] = gb2312_char & 0xFF;
    }
    
    std::string utf8_result = gb2312ToUtf8(gb_bytes, strlen(gb_bytes));
    
    for (char c : utf8_result) {
        result.push_back(static_cast<uint8_t>(c));
    }
    
    return result;
}

uint16_t TextConverter::utf8CharToGb2312(const uint8_t* utf8_bytes, int& bytes_used) {
    bytes_used = getUtf8CharLength(utf8_bytes[0]);
    if (bytes_used == 0) {
        return 0;
    }
    
    // Convert UTF-8 character using iconv
    std::string utf8_char(reinterpret_cast<const char*>(utf8_bytes), bytes_used);
    std::string gb2312_result = utf8ToGb2312(utf8_char);
    
    if (gb2312_result.empty()) {
        return 0;
    }
    
    if (gb2312_result.length() == 1) {
        return static_cast<uint8_t>(gb2312_result[0]);
    } else if (gb2312_result.length() == 2) {
        return (static_cast<uint8_t>(gb2312_result[0]) << 8) | static_cast<uint8_t>(gb2312_result[1]);
    }
    
    return 0;
}

int TextConverter::getUtf8CharLength(uint8_t first_byte) {
    if ((first_byte & 0x80) == 0) {
        return 1; // ASCII character
    } else if ((first_byte & 0xE0) == 0xC0) {
        return 2; // 2-byte character
    } else if ((first_byte & 0xF0) == 0xE0) {
        return 3; // 3-byte character
    } else if ((first_byte & 0xF8) == 0xF0) {
        return 4; // 4-byte character
    }
    
    return 0; // Invalid UTF-8 first byte
}