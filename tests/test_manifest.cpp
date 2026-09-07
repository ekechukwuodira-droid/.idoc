#include <doctest/doctest.h>
#include "idoc/container/manifest.hpp"

using namespace idoc;

TEST_CASE("manifest: json round-trip preserves all fields") {
    Manifest m;
    m.format_version = "1.0";
    m.created_by = "LightOffice 1.0.0";
    m.created_at = "2026-08-30T12:00:00Z";
    m.last_modified_by = "LightOffice 1.0.0";
    m.document_id = "b3f1e2a0-test";
    m.resource_count = 14;
    m.incremental_save_generation = 7;

    ManifestBlockEntry b0;
    b0.id = 0; b0.type = "metadata"; b0.version = 1; b0.offset = 64; b0.length = 512;
    ManifestBlockEntry b1;
    b1.id = 5; b1.type = "content"; b1.version = 1; b1.offset = 1024; b1.length = 88213;
    m.blocks = {b0, b1};

    std::string json = m.to_json();
    Manifest m2 = Manifest::from_json(json);

    CHECK(m2.format_version == m.format_version);
    CHECK(m2.created_by == m.created_by);
    CHECK(m2.created_at == m.created_at);
    CHECK(m2.last_modified_by == m.last_modified_by);
    CHECK(m2.document_id == m.document_id);
    CHECK(m2.resource_count == m.resource_count);
    CHECK(m2.incremental_save_generation == m.incremental_save_generation);
    REQUIRE(m2.blocks.size() == 2);
    CHECK(m2.blocks[0].type == "metadata");
    CHECK(m2.blocks[1].offset == 1024);
    CHECK(m2.blocks[1].length == 88213);
}

TEST_CASE("manifest: empty block list round-trips") {
    Manifest m;
    m.document_id = "empty-doc";
    std::string json = m.to_json();
    Manifest m2 = Manifest::from_json(json);
    CHECK(m2.blocks.empty());
    CHECK(m2.document_id == "empty-doc");
}

TEST_CASE("manifest: malformed json throws") {
    CHECK_THROWS(Manifest::from_json("{ not valid json"));
}
