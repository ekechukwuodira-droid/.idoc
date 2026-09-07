#include <doctest/doctest.h>
#include "idoc/serde/styles_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

TEST_CASE("styles serde: single style round-trip") {
    model::StyleDefinition def;
    def.style_id = "Normal";
    def.display_name = "Normal";
    def.type = model::StyleType::kParagraph;
    def.is_default = true;
    def.quick_style = true;

    model::Styles s;
    s.definitions = {def};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    CHECK(s2 == s);
}

TEST_CASE("styles serde: inheritance chain fields (based_on, next_style) round-trip") {
    model::StyleDefinition heading1;
    heading1.style_id = "Heading1";
    heading1.display_name = "Heading 1";
    heading1.type = model::StyleType::kParagraph;
    heading1.based_on = "Normal";
    heading1.next_style = "Normal";
    heading1.is_default = false;
    heading1.quick_style = true;

    model::Styles s;
    s.definitions = {heading1};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    REQUIRE(s2.definitions.size() == 1);
    CHECK(s2.definitions[0].based_on.value() == "Normal");
    CHECK(s2.definitions[0].next_style.value() == "Normal");
    CHECK(s2 == s);
}

TEST_CASE("styles serde: based_on/next_style absent round-trip as nullopt") {
    model::StyleDefinition def;
    def.style_id = "Normal";
    def.display_name = "Normal";
    def.type = model::StyleType::kParagraph;
    // based_on, next_style deliberately left unset

    model::Styles s;
    s.definitions = {def};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    CHECK_FALSE(s2.definitions[0].based_on.has_value());
    CHECK_FALSE(s2.definitions[0].next_style.has_value());
}

TEST_CASE("styles serde: multiple style types round-trip") {
    model::StyleDefinition para;
    para.style_id = "Normal"; para.display_name = "Normal"; para.type = model::StyleType::kParagraph;

    model::StyleDefinition character;
    character.style_id = "Emphasis"; character.display_name = "Emphasis";
    character.type = model::StyleType::kCharacter; character.based_on = "DefaultParagraphFont";

    model::StyleDefinition table;
    table.style_id = "TableGrid"; table.display_name = "Table Grid";
    table.type = model::StyleType::kTable;

    model::Styles s;
    s.definitions = {para, character, table};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    REQUIRE(s2.definitions.size() == 3);
    CHECK(s2.definitions[0].type == model::StyleType::kParagraph);
    CHECK(s2.definitions[1].type == model::StyleType::kCharacter);
    CHECK(s2.definitions[2].type == model::StyleType::kTable);
    CHECK(s2 == s);
}

TEST_CASE("styles serde: empty style list round-trips") {
    model::Styles s;
    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);
    CHECK(s2.definitions.empty());
}

TEST_CASE("styles serde: unknown field within a StyleDefinition is skipped") {
    model::StyleDefinition def;
    def.style_id = "Normal";
    def.display_name = "Normal";
    def.type = model::StyleType::kParagraph;

    model::Styles s;
    s.definitions = {def};
    auto payload = serde::serialize_styles(s);

    // A future minor version adds a field this reader doesn't know about,
    // nested inside the single StyleDefinition record. We rebuild the
    // outer record with an extra inner field appended to its payload.
    auto outer_records = tlv::parse_records(payload);
    REQUIRE(outer_records.size() == 1);
    std::vector<uint8_t> patched_inner = outer_records[0].payload;
    tlv::write_record(patched_inner, 999, 1, {0x01});

    std::vector<uint8_t> patched_payload;
    tlv::write_record(patched_payload, outer_records[0].header.type_id,
                       outer_records[0].header.schema_version, patched_inner);

    auto s2 = serde::deserialize_styles(patched_payload);
    REQUIRE(s2.definitions.size() == 1);
    CHECK(s2.definitions[0].style_id == "Normal");
}

TEST_CASE("styles serde: unknown top-level record type is skipped, not fatal") {
    model::Styles s;
    auto payload = serde::serialize_styles(s);
    tlv::write_record(payload, 999, 1, {0xFF}); // unrecognized top-level record type

    auto s2 = serde::deserialize_styles(payload);
    CHECK(s2.definitions.empty()); // the unknown record contributed nothing, didn't throw
}
