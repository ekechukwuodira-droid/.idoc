#include <doctest/doctest.h>
#include "idoc/container/file_header.hpp"

#include <stdexcept>

using namespace idoc;

TEST_CASE("file header: default construct is exactly kFileHeaderSize bytes") {
    FileHeader h;
    auto bytes = h.serialize();
    CHECK(bytes.size() == kFileHeaderSize);
    CHECK(kFileHeaderSize == 64);
}

TEST_CASE("file header: round-trip preserves all fields") {
    FileHeader h;
    h.format_major = 1;
    h.format_minor = 3;
    h.set_flag(kFlagIncrementalSavePending, true);
    h.manifest_offset = 64;
    h.manifest_length = 512;
    h.block_directory_offset = 576;
    h.resource_area_offset = 100000;
    h.file_length = 200000;

    auto bytes = h.serialize();
    FileHeader h2 = FileHeader::deserialize(bytes);

    CHECK(h2.format_major == h.format_major);
    CHECK(h2.format_minor == h.format_minor);
    CHECK(h2.header_flags == h.header_flags);
    CHECK(h2.has_flag(kFlagIncrementalSavePending) == true);
    CHECK(h2.has_flag(kFlagEncrypted) == false);
    CHECK(h2.manifest_offset == h.manifest_offset);
    CHECK(h2.manifest_length == h.manifest_length);
    CHECK(h2.block_directory_offset == h.block_directory_offset);
    CHECK(h2.resource_area_offset == h.resource_area_offset);
    CHECK(h2.file_length == h.file_length);
}

TEST_CASE("file header: bad magic throws") {
    FileHeader h;
    auto bytes = h.serialize();
    bytes[0] = 'X'; // corrupt magic
    CHECK_THROWS_AS(FileHeader::deserialize(bytes), std::runtime_error);
}

TEST_CASE("file header: truncated buffer throws") {
    std::vector<uint8_t> bytes(10, 0);
    CHECK_THROWS_AS(FileHeader::deserialize(bytes), std::out_of_range);
}
