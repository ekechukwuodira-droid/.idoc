#include <doctest/doctest.h>
#include "idoc/container/crc64.hpp"

#include <cstring>

TEST_CASE("crc64 of empty input") {
    uint64_t c = idoc::crc64(nullptr, 0);
    // CRC of empty input with all-ones init/xorout is 0.
    CHECK(c == 0ULL);
}

TEST_CASE("crc64 known test vector: '123456789'") {
    const char* s = "123456789";
    uint64_t c = idoc::crc64(reinterpret_cast<const uint8_t*>(s), std::strlen(s));
    // Official CRC-64/XZ check value for the ASCII string "123456789".
    CHECK(c == 0x995DC9BBDF1939FAULL);
}

TEST_CASE("crc64 is sensitive to single-byte changes") {
    std::vector<uint8_t> a = {1, 2, 3, 4, 5};
    std::vector<uint8_t> b = {1, 2, 3, 4, 6};
    CHECK(idoc::crc64(a) != idoc::crc64(b));
}

TEST_CASE("crc64 is deterministic") {
    std::vector<uint8_t> data = {10, 20, 30, 40, 50, 60, 70};
    CHECK(idoc::crc64(data) == idoc::crc64(data));
}
