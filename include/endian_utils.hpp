#ifndef ENDIAN_UTILS_HPP
#define ENDIAN_UTILS_HPP

#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

namespace boomphf
{

/// Runtime endianness check
[[nodiscard]] inline bool is_system_little_endian() noexcept
{
	static const bool little_endian = []() {
		constexpr uint32_t test = 0x01020304;
		const auto* bytes = reinterpret_cast<const uint8_t*>(&test);
		return bytes[0] == 0x04;
	}();
	return little_endian;
}

/// Byte swap functions for different integer sizes
[[nodiscard]] constexpr uint16_t byteswap16(uint16_t value) noexcept { return (value >> 8) | (value << 8); }

[[nodiscard]] constexpr uint32_t byteswap32(uint32_t value) noexcept
{
	return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) |
	       ((value << 24) & 0xFF000000);
}

[[nodiscard]] constexpr uint64_t byteswap64(uint64_t value) noexcept
{
	return ((value >> 56) & 0x00000000000000FFULL) | ((value >> 40) & 0x000000000000FF00ULL) |
	       ((value >> 24) & 0x0000000000FF0000ULL) | ((value >> 8) & 0x00000000FF000000ULL) |
	       ((value << 8) & 0x000000FF00000000ULL) | ((value << 24) & 0x0000FF0000000000ULL) |
	       ((value << 40) & 0x00FF000000000000ULL) | ((value << 56) & 0xFF00000000000000ULL);
}

/// Convert value to little-endian format for saving
template <typename T> [[nodiscard]] inline T to_little_endian(T value) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
	static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
	              "Only 1, 2, 4, and 8 byte types are supported");

	if (is_system_little_endian())
	{
		return value;
	}

	// Big-endian system - swap bytes using if constexpr for compile-time dispatch
	if constexpr (sizeof(T) == 1)
	{
		return value;
	}
	else if constexpr (sizeof(T) == 2)
	{
		T result;
		uint16_t temp;
		std::memcpy(&temp, &value, sizeof(T));
		temp = byteswap16(temp);
		std::memcpy(&result, &temp, sizeof(T));
		return result;
	}
	else if constexpr (sizeof(T) == 4)
	{
		T result;
		uint32_t temp;
		std::memcpy(&temp, &value, sizeof(T));
		temp = byteswap32(temp);
		std::memcpy(&result, &temp, sizeof(T));
		return result;
	}
	else
	{ // sizeof(T) == 8
		T result;
		uint64_t temp;
		std::memcpy(&temp, &value, sizeof(T));
		temp = byteswap64(temp);
		std::memcpy(&result, &temp, sizeof(T));
		return result;
	}
}

/// Convert value from little-endian format for loading
template <typename T> [[nodiscard]] inline T from_little_endian(T value) noexcept { return to_little_endian(value); }

/// Write value to stream in little-endian format
template <typename T> inline void write_le(std::ostream& os, const T& value)
{
	static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
	T le_value = to_little_endian(value);
	os.write(reinterpret_cast<const char*>(&le_value), sizeof(T));
}

/// Read value from stream in little-endian format
template <typename T> inline void read_le(std::istream& is, T& value)
{
	static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");
	T le_value;
	is.read(reinterpret_cast<char*>(&le_value), sizeof(T));
	value = from_little_endian(le_value);
}

/// Write array to stream in little-endian format
template <typename T> inline void write_le_array(std::ostream& os, const T* data, size_t count)
{
	static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");

	if (is_system_little_endian())
	{
		// Fast path: already little-endian, write directly
		os.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
	}
	else
	{
		// Slow path: convert each element
		for (size_t i = 0; i < count; ++i)
		{
			write_le(os, data[i]);
		}
	}
}

/// Read array from stream in little-endian format
template <typename T> inline void read_le_array(std::istream& is, T* data, size_t count)
{
	static_assert(std::is_arithmetic_v<T>, "Type must be arithmetic");

	if (is_system_little_endian())
	{
		// Fast path: already little-endian, read directly
		is.read(reinterpret_cast<char*>(data), sizeof(T) * count);
	}
	else
	{
		// Slow path: convert each element
		for (size_t i = 0; i < count; ++i)
		{
			read_le(is, data[i]);
		}
	}
}

} // namespace boomphf

#endif // ENDIAN_UTILS_HPP
