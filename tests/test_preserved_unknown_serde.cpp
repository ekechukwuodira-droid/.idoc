#include <doctest/doctest.h>
#include "idoc/serde/preserved_unknown_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

TEST_CASE("preserved_unknown serde: full round-trip") {
    model::PreservedNode node = serde::make_preserved_node(
        "para-42", "docx-ooxml", "/w:document/w:body/w:p[3]/w:customXml",
        std::vector<uint8_t>{0x3C, 0x77, 0x3A, 0x63, 0x75, 0x73, 0x74, 0x6F, 0x6D});

    model::PreservedUnknown pu;
    pu.nodes = {node};

    auto payload = serde::serialize_preserved_unknown(pu);
    auto pu2 = serde::deserialize_preserved_unknown(payload);

    CHECK(pu2 == pu);
}

TEST_CASE("preserved_unknown serde: document-level anchor (anchor_node_id == \"document\")") {
    model::PreservedNode node = serde::make_preserved_node(
        "document", "docx-ooxml", "/w:document/w:settings/w:customExtension",
        std::vector<uint8_t>{0x01, 0x02});

    model::PreservedUnknown pu;
    pu.nodes = {node};

    auto payload = serde::serialize_preserved_unknown(pu);
    auto pu2 = serde::deserialize_preserved_unknown(payload);

    REQUIRE(pu2.nodes.size() == 1);
    CHECK(pu2.nodes[0].anchor_node_id == "document");
}

TEST_CASE("preserved_unknown serde: make_preserved_node computes a real CRC64, not zero") {
    model::PreservedNode node = serde::make_preserved_node(
        "run-1", "docx-ooxml", "/w:r/w:custom", std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF});

    CHECK(node.checksum != 0);
    CHECK(serde::verify_preserved_node_checksum(node));
}

TEST_CASE("preserved_unknown serde: verify_preserved_node_checksum detects tampering") {
    model::PreservedNode node = serde::make_preserved_node(
        "run-1", "docx-ooxml", "/w:r/w:custom", std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF});

    CHECK(serde::verify_preserved_node_checksum(node));

    node.raw_payload.push_back(0xFF); // simulate corruption after the checksum was computed
    CHECK_FALSE(serde::verify_preserved_node_checksum(node));
}

TEST_CASE("preserved_unknown serde: raw_payload with embedded null bytes round-trips exactly") {
    std::vector<uint8_t> payload_with_nulls = {0x00, 0x01, 0x00, 0xFF, 0x00};
    model::PreservedNode node = serde::make_preserved_node(
        "para-1", "docx-ooxml", "/w:p/w:custom", payload_with_nulls);

    model::PreservedUnknown pu;
    pu.nodes = {node};

    auto payload = serde::serialize_preserved_unknown(pu);
    auto pu2 = serde::deserialize_preserved_unknown(payload);

    CHECK(pu2.nodes[0].raw_payload == payload_with_nulls);
}

TEST_CASE("preserved_unknown serde: multiple nodes preserve order") {
    model::PreservedUnknown pu;
    pu.nodes.push_back(serde::make_preserved_node("para-1", "docx-ooxml", "/p1", {0x01}));
    pu.nodes.push_back(serde::make_preserved_node("para-2", "docx-ooxml", "/p2", {0x02}));

    auto payload = serde::serialize_preserved_unknown(pu);
    auto pu2 = serde::deserialize_preserved_unknown(payload);

    REQUIRE(pu2.nodes.size() == 2);
    CHECK(pu2.nodes[0].anchor_node_id == "para-1");
    CHECK(pu2.nodes[1].anchor_node_id == "para-2");
}

TEST_CASE("preserved_unknown serde: empty store round-trips") {
    model::PreservedUnknown pu;
    auto payload = serde::serialize_preserved_unknown(pu);
    auto pu2 = serde::deserialize_preserved_unknown(payload);
    CHECK(pu2.nodes.empty());
}

TEST_CASE("preserved_unknown serde: unknown top-level record type is skipped, not fatal") {
    model::PreservedUnknown pu;
    pu.nodes.push_back(serde::make_preserved_node("para-1", "docx-ooxml", "/p1", {0x01}));
    auto payload = serde::serialize_preserved_unknown(pu);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto pu2 = serde::deserialize_preserved_unknown(payload);
    REQUIRE(pu2.nodes.size() == 1);
    CHECK(pu2.nodes[0].anchor_node_id == "para-1");
}
