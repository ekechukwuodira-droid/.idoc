#pragma once
// CRC-64/XZ (poly 0xC96C5795D7870F42, reflected, init/xorout all-ones).
// Same variant used by xz/7-zip -- a well-tested, widely implemented choice
// for a whole-file trailing checksum. §1.1 of the spec.

#include <cstdint>
#include <cstddef>
#include <vector>

namespace idoc {

uint64_t crc64(const uint8_t* data, size_t len);

inline uint64_t crc64(const std::vector<uint8_t>& data) {
    return crc64(data.data(), data.size());
}

} // namespace idoc
