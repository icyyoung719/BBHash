#ifndef ENDIAN_UTILS_HPP
#define ENDIAN_UTILS_HPP

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace boomphf {

// Runtime endianness check
inline bool is_system_little_endian() {
    uint32_t test = 0x01020304;
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&test);
    return bytes[0] == 0x04;
}

// Byte swap functions
inline uint16_t byteswap16(uint16_t value) {
    return (value >> 8) | (value << 8);
}

inline uint32_t byteswap32(uint32_t value) {
    return ((value >> 24) & 0x000000FF) |
           ((value >> 8)  & 0x0000FF00) |
           ((value << 8)  & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

inline uint64_t byteswap64(uint64_t value) {
    return ((value >> 56) & 0x00000000000000FFULL) |
           ((value >> 40) & 0x000000000000FF00ULL) |
           ((value >> 24) & 0x0000000000FF0000ULL) |
           ((value >> 8)  & 0x00000000FF000000ULL) |
           ((value << 8)  & 0x000000FF00000000ULL) |
           ((value << 24) & 0x0000FF0000000000ULL) |
           ((value << 40) & 0x00FF000000000000ULL) |
           ((value << 56) & 0xFF00000000000000ULL);
}

// Convert to little-endian (for saving)
template<typename T>
inline T to_little_endian(T value) {
    static_assert(std::is_arithmetic<T>::value, "Type must be arithmetic");
    
    if (is_system_little_endian()) {
        return value;
    }
    
    // Big-endian system, need to swap bytes
    if (sizeof(T) == 1) {
        return value;
    } else if (sizeof(T) == 2) {
        T result;
        uint16_t temp;
        std::memcpy(&temp, &value, sizeof(T));
        temp = byteswap16(temp);
        std::memcpy(&result, &temp, sizeof(T));
        return result;
    } else if (sizeof(T) == 4) {
        T result;
        uint32_t temp;
        std::memcpy(&temp, &value, sizeof(T));
        temp = byteswap32(temp);
        std::memcpy(&result, &temp, sizeof(T));
        return result;
    } else if (sizeof(T) == 8) {
        T result;
        uint64_t temp;
        std::memcpy(&temp, &value, sizeof(T));
        temp = byteswap64(temp);
        std::memcpy(&result, &temp, sizeof(T));
        return result;
    } else {
        // Unsupported size, just return as is
        return value;
    }
}

// Convert from little-endian (for loading)
template<typename T>
inline T from_little_endian(T value) {
    // Converting from little-endian is the same as converting to little-endian
    return to_little_endian(value);
}

// Write value to stream in little-endian format
template<typename T>
inline void write_le(std::ostream& os, const T& value) {
    static_assert(std::is_arithmetic<T>::value, "Type must be arithmetic");
    T le_value = to_little_endian(value);
    os.write(reinterpret_cast<const char*>(&le_value), sizeof(T));
}

// Read value from stream in little-endian format
template<typename T>
inline void read_le(std::istream& is, T& value) {
    static_assert(std::is_arithmetic<T>::value, "Type must be arithmetic");
    T le_value;
    is.read(reinterpret_cast<char*>(&le_value), sizeof(T));
    value = from_little_endian(le_value);
}

// Write array to stream in little-endian format
template<typename T>
inline void write_le_array(std::ostream& os, const T* data, size_t count) {
    static_assert(std::is_arithmetic<T>::value, "Type must be arithmetic");
    
    if (is_system_little_endian()) {
        // Fast path: already little-endian, write directly
        os.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
    } else {
        // Slow path: convert each element
        for (size_t i = 0; i < count; ++i) {
            write_le(os, data[i]);
        }
    }
}

// Read array from stream in little-endian format
template<typename T>
inline void read_le_array(std::istream& is, T* data, size_t count) {
    static_assert(std::is_arithmetic<T>::value, "Type must be arithmetic");
    
    if (is_system_little_endian()) {
        // Fast path: already little-endian, read directly
        is.read(reinterpret_cast<char*>(data), sizeof(T) * count);
    } else {
        // Slow path: convert each element
        for (size_t i = 0; i < count; ++i) {
            read_le(is, data[i]);
        }
    }
}

} // namespace boomphf

#endif // ENDIAN_UTILS_HPP
