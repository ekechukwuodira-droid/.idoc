#include <doctest/doctest.h>
#include "idoc/serde/resources_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::ResourceEntry make_sample_entry() {
    model::ResourceEntry e;
    e.resource_id = "res-1";
    e.original_filename = "cover-art.png";
    e.mime_type = "image/png";
    e.blob_offset = 4096;
    e.blob_length = 102400;
    e.natural_size = model::Size2D{1200, 1600};
    e.sha256 = "9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08";
    return e;
}

} // namespace

TEST_CASE("resources serde: full round-trip") {
    model::ResourceIndex idx;
    idx.entries = {make_sample_entry()};

    auto payload = serde::serialize_resource_index(idx);
    auto idx2 = serde::deserialize_resource_index(payload);

    CHECK(idx2 == idx);
}

TEST_CASE("resources serde: original_filename and natural_size absent round-trip as nullopt") {
    model::ResourceEntry e;
    e.resource_id = "res-font-1";
    e.mime_type = "font/ttf";
    e.blob_offset = 0;
    e.blob_length = 50000;
    e.sha256 = "abc123";
    // original_filename, natural_size deliberately left unset (fonts have no "natural size")

    model::ResourceIndex idx;
    idx.entries = {e};

    auto payload = serde::serialize_resource_index(idx);
    auto idx2 = serde::deserialize_resource_index(payload);

    REQUIRE(idx2.entries.size() == 1);
    CHECK_FALSE(idx2.entries[0].original_filename.has_value());
    CHECK_FALSE(idx2.entries[0].natural_size.has_value());
    CHECK(idx2 == idx);
}

TEST_CASE("resources serde: large blob offset/length (uint64) round-trips exactly") {
    model::ResourceEntry e = make_sample_entry();
    e.blob_offset = 0xFFFFFFFF00ULL; // beyond 32-bit range
    e.blob_length = 0x100000000ULL; // exactly 4GB

    model::ResourceIndex idx;
    idx.entries = {e};

    auto payload = serde::serialize_resource_index(idx);
    auto idx2 = serde::deserialize_resource_index(payload);

    CHECK(idx2.entries[0].blob_offset == 0xFFFFFFFF00ULL);
    CHECK(idx2.entries[0].blob_length == 0x100000000ULL);
}

TEST_CASE("resources serde: multiple entries preserve order and dedup-relevant sha256") {
    model::ResourceEntry e1 = make_sample_entry();
    e1.resource_id = "res-1";
    model::ResourceEntry e2 = make_sample_entry();
    e2.resource_id = "res-2";
    e2.sha256 = e1.sha256; // same content, different placements -- dedup case per §11's prose

    model::ResourceIndex idx;
    idx.entries = {e1, e2};

    auto payload = serde::serialize_resource_index(idx);
    auto idx2 = serde::deserialize_resource_index(payload);

    REQUIRE(idx2.entries.size() == 2);
    CHECK(idx2.entries[0].resource_id == "res-1");
    CHECK(idx2.entries[1].resource_id == "res-2");
    CHECK(idx2.entries[0].sha256 == idx2.entries[1].sha256);
}

TEST_CASE("resources serde: empty ResourceIndex round-trips") {
    model::ResourceIndex idx;
    auto payload = serde::serialize_resource_index(idx);
    auto idx2 = serde::deserialize_resource_index(payload);
    CHECK(idx2.entries.empty());
}

TEST_CASE("resources serde: unknown field within an entry is skipped") {
    model::ResourceIndex idx;
    idx.entries = {make_sample_entry()};
    auto payload = serde::serialize_resource_index(idx);

    auto outer_records = tlv::parse_records(payload);
    REQUIRE(outer_records.size() == 1);
    std::vector<uint8_t> patched_inner = outer_records[0].payload;
    tlv::write_record(patched_inner, 999, 1, {0x01});

    std::vector<uint8_t> patched_payload;
    tlv::write_record(patched_payload, outer_records[0].header.type_id,
                       outer_records[0].header.schema_version, patched_inner);

    auto idx2 = serde::deserialize_resource_index(patched_payload);
    REQUIRE(idx2.entries.size() == 1);
    CHECK(idx2.entries[0].resource_id == "res-1");
}

TEST_CASE("resources serde: unknown top-level record type is skipped, not fatal") {
    model::ResourceIndex idx;
    auto payload = serde::serialize_resource_index(idx);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto idx2 = serde::deserialize_resource_index(payload);
    CHECK(idx2.entries.empty());
}
