# Endianness-Safe Serialization

This document describes the changes made to ensure BBHash serialization is cross-architecture compatible.

## Problem

The original BBHash implementation used direct `std::ostream::write()` and `std::istream::read()` operations on binary data without considering endianness. This caused issues when trying to load `.mphf` files created on one architecture (e.g., little-endian x86) on a different architecture (e.g., big-endian systems).

## Solution

All save/load operations now use **little-endian format** for serialization, ensuring that `.mphf` files are portable across all architectures.

### Changes Made

1. **endian_utils.hpp** - New header with endianness conversion utilities:
   - `is_system_little_endian()` - Runtime endianness detection
   - `byteswap16/32/64()` - Byte swapping functions
   - `to_little_endian/from_little_endian()` - Conversion templates
   - `write_le/read_le()` - Endianness-safe I/O for single values
   - `write_le_array/read_le_array()` - Endianness-safe I/O for arrays

2. **bitvector.hpp** - Updated save/load methods:
   - All numeric fields (_size, _nchar, etc.) use endianness-safe I/O
   - Bit arrays properly converted from atomic<uint64_t> before saving
   - Rank arrays use array I/O functions

3. **BooPHF.h** - Updated save/load methods:
   - All numeric fields (_gamma, _nb_levels, etc.) use endianness-safe I/O
   - Final hash map entries saved with proper endianness

4. **C++ Standard** - Updated from C++11 to C++17:
   - Required for proper template features used in endian_utils.hpp

### Performance

- **Little-endian systems** (x86, x86-64, ARM in little-endian mode): No performance impact - fast path bypasses conversion
- **Big-endian systems**: Small overhead from byte swapping during I/O operations

### Testing

Run the comprehensive test suite:

```bash
make test_endian
./test_endian
```

The test validates save/load operations with various MPHF configurations.

### Compatibility

- **Forward compatible**: New `.mphf` files use little-endian format
- **Breaking change**: Old `.mphf` files created before this change may not load correctly on different endianness systems
- **Within-architecture**: Old files continue to work on the same architecture they were created on

### Usage

No changes required to user code. All endianness handling is transparent:

```cpp
// Build and save
boophf_t* bphf = new boomphf::mphf<uint64_t,hasher_t>(nelem, data, nthreads);
std::ofstream os("file.mphf", std::ios::binary);
bphf->save(os);  // Automatically uses little-endian

// Load
boophf_t* loaded = new boomphf::mphf<uint64_t,hasher_t>();
std::ifstream is("file.mphf", std::ios::binary);
loaded->load(is);  // Automatically converts from little-endian
```

## Technical Details

### Byte Order

All multi-byte values are stored in **little-endian** (least significant byte first) format:
- `uint16_t`, `uint32_t`, `uint64_t`
- `double` (IEEE 754 format with little-endian byte order)
- All numeric fields in bitVector and mphf classes

### Implementation Strategy

The implementation uses compile-time type checking and runtime endianness detection:

```cpp
template<typename T>
inline void write_le(std::ostream& os, const T& value) {
    static_assert(std::is_arithmetic<T>::value, "Type must be arithmetic");
    T le_value = to_little_endian(value);
    os.write(reinterpret_cast<const char*>(&le_value), sizeof(T));
}
```

On little-endian systems, `to_little_endian()` becomes a no-op at runtime, maintaining performance.
