#include "idoc/container/crc64.hpp"

#include <array>

namespace idoc {

namespace {

constexpr uint64_t kPoly = 0xC96C5795D7870F42ULL; // reflected CRC-64/XZ polynomial

std::array<uint64_t, 256> build_table() {
    std::array<uint64_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint64_t c = i;
        for (int j = 0; j < 8; ++j) {
            if (c & 1ULL) {
                c = kPoly ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }
        table[i] = c;
    }
    return table;
}

const std::array<uint64_t, 256>& table() {
    static const std::array<uint64_t, 256> t = build_table();
    return t;
}

} // namespace

uint64_t crc64(const uint8_t* data, size_t len) {
    const auto& t = table();
    uint64_t crc = ~0ULL; // init: all-ones
    for (size_t i = 0; i < len; ++i) {
        crc = t[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ ~0ULL; // xorout: all-ones
}

} // namespace idoc
