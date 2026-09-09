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

TEST_CASE("styles serde: paragraph_props round-trips with real ParagraphProperties") {
    model::StyleDefinition def;
    def.style_id = "Heading1";
    def.display_name = "Heading 1";
    def.type = model::StyleType::kParagraph;

    model::ParagraphProperties pp;
    pp.alignment = model::Alignment::kLeft;
    pp.spacing = model::Spacing{480, 240, 0, model::LineRule::kSingle};
    pp.keep_with_next = true;
    // indent, keep_lines_together, page_break_before deliberately left unset
    def.paragraph_props = pp;

    model::Styles s;
    s.definitions = {def};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    REQUIRE(s2.definitions.size() == 1);
    REQUIRE(s2.definitions[0].paragraph_props.has_value());
    const auto& pp2 = s2.definitions[0].paragraph_props.value();
    CHECK(pp2.alignment.value() == model::Alignment::kLeft);
    CHECK(pp2.keep_with_next.value() == true);
    CHECK_FALSE(pp2.indent.has_value());
    CHECK_FALSE(pp2.keep_lines_together.has_value());
    CHECK(s2 == s);
}

TEST_CASE("styles serde: run_props round-trips with real RunProperties") {
    model::StyleDefinition def;
    def.style_id = "Emphasis";
    def.display_name = "Emphasis";
    def.type = model::StyleType::kCharacter;

    model::RunProperties rp;
    rp.italic = true;
    rp.font = model::FontRef{"Georgia", std::nullopt};
    // bold, size_pt, color, etc. deliberately left unset
    def.run_props = rp;

    model::Styles s;
    s.definitions = {def};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    REQUIRE(s2.definitions.size() == 1);
    REQUIRE(s2.definitions[0].run_props.has_value());
    const auto& rp2 = s2.definitions[0].run_props.value();
    CHECK(rp2.italic.value() == true);
    CHECK(rp2.font.value().family == "Georgia");
    CHECK_FALSE(rp2.bold.has_value());
    CHECK_FALSE(rp2.size_pt.has_value());
}

TEST_CASE("styles serde: paragraph_props and run_props absent round-trip as nullopt") {
    model::StyleDefinition def;
    def.style_id = "Normal";
    def.display_name = "Normal";
    def.type = model::StyleType::kParagraph;
    // paragraph_props, run_props both deliberately left unset

    model::Styles s;
    s.definitions = {def};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    CHECK_FALSE(s2.definitions[0].paragraph_props.has_value());
    CHECK_FALSE(s2.definitions[0].run_props.has_value());
}

TEST_CASE("styles serde: a style can carry both paragraph_props and run_props at once") {
    // Real word processors do this: a paragraph style like "Heading 1"
    // sets both paragraph-level formatting (spacing, keep_with_next) AND
    // default character formatting for text typed in that style (bold,
    // a larger size) at the same time.
    model::StyleDefinition def;
    def.style_id = "Heading1";
    def.display_name = "Heading 1";
    def.type = model::StyleType::kParagraph;

    model::ParagraphProperties pp;
    pp.keep_with_next = true;
    def.paragraph_props = pp;

    model::RunProperties rp;
    rp.bold = true;
    rp.size_pt = 16.0f;
    def.run_props = rp;

    model::Styles s;
    s.definitions = {def};

    auto payload = serde::serialize_styles(s);
    auto s2 = serde::deserialize_styles(payload);

    REQUIRE(s2.definitions[0].paragraph_props.has_value());
    REQUIRE(s2.definitions[0].run_props.has_value());
    CHECK(s2.definitions[0].paragraph_props->keep_with_next.value() == true);
    CHECK(s2.definitions[0].run_props->bold.value() == true);
    CHECK(s2.definitions[0].run_props->size_pt.value() == doctest::Approx(16.0f));
}
