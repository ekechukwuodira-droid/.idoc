#include <doctest/doctest.h>
#include "idoc/container/tlv.hpp"

#include <stdexcept>

using namespace idoc;

TEST_CASE("tlv: single record round-trip") {
    std::vector<uint8_t> buf;
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    tlv::write_record(buf, 42, 3, payload);

    auto records = tlv::parse_records(buf);
    REQUIRE(records.size() == 1);
    CHECK(records[0].header.type_id == 42);
    CHECK(records[0].header.schema_version == 3);
    CHECK(records[0].header.length == 4);
    CHECK(records[0].payload == payload);
}

TEST_CASE("tlv: multiple records round-trip in order") {
    std::vector<uint8_t> buf;
    tlv::write_record(buf, 1, 1, {1, 2, 3});
    tlv::write_record(buf, 2, 1, {});
    tlv::write_record(buf, 3, 1, {9, 9});

    auto records = tlv::parse_records(buf);
    REQUIRE(records.size() == 3);
    CHECK(records[0].header.type_id == 1);
    CHECK(records[1].header.type_id == 2);
    CHECK(records[1].payload.empty());
    CHECK(records[2].header.type_id == 3);
}

TEST_CASE("tlv: unknown type_id is preserved as opaque bytes, not lost") {
    // Simulates a v2-authored file being read by a v1-shaped consumer: the
    // consumer doesn't know type_id 999, but parse_records still returns
    // it with its raw payload intact -- the "skip by length not by name"
    // contract from §1.4. It's the *caller's* job to decide what to do
    // with an unrecognized type_id (ignore it, or mirror it into
    // PreservedUnknown); this layer must not silently drop bytes.
    std::vector<uint8_t> buf;
    tlv::write_record(buf, 999, 7, {0xAA, 0xBB, 0xCC});

    auto records = tlv::parse_records(buf);
    REQUIRE(records.size() == 1);
    CHECK(records[0].header.type_id == 999);
    CHECK(records[0].payload == std::vector<uint8_t>{0xAA, 0xBB, 0xCC});

    CHECK(tlv::find_record(records, 999) != nullptr);
    CHECK(tlv::find_record(records, 1) == nullptr);
}

TEST_CASE("tlv: truncated payload throws rather than reading garbage") {
    std::vector<uint8_t> buf;
    // Hand-craft a header claiming a 10-byte payload but only supply 2 bytes.
    buf.push_back(1); buf.push_back(0); buf.push_back(0); buf.push_back(0); // type_id = 1
    buf.push_back(0); buf.push_back(0);                                     // schema_version = 0
    buf.push_back(10); buf.push_back(0); buf.push_back(0); buf.push_back(0); // length = 10
    buf.push_back(0xAA); buf.push_back(0xBB);                                // only 2 payload bytes

    CHECK_THROWS_AS(tlv::parse_records(buf), std::out_of_range);
}
