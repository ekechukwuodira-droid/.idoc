#include <doctest/doctest.h>
#include "idoc/serde/sections_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::Section make_sample_section() {
    model::Section s;
    s.section_id = "sec-1";

    s.page_setup.page_size = {12240, 15840}; // US Letter, twips
    s.page_setup.margins = {1440, 1440, 1440, 1440, 0};
    s.page_setup.orientation = model::Orientation::kPortrait;
    s.page_setup.columns = {1, 0, true, false};
    s.page_setup.different_first_page = true;
    s.page_setup.different_odd_even = false;
    s.page_setup.paper_source = "Tray 1";

    model::HeaderFooterContent default_header;
    default_header.content = {model::ContentRef{model::ContentType::kParagraph, "para-1"}};
    s.headers.default_ = default_header;

    model::HeaderFooterContent first_footer;
    first_footer.content = {model::ContentRef{model::ContentType::kParagraph, "para-2"}};
    s.footers.first = first_footer;

    model::NumberingRestart restart;
    restart.restart = true;
    restart.start_at = 1;
    s.page_number_restart = restart;

    s.content = {model::ContentRef{model::ContentType::kParagraph, "para-3"},
                 model::ContentRef{model::ContentType::kTable, "table-1"}};

    return s;
}

} // namespace

TEST_CASE("sections serde: full round-trip") {
    model::Sections sections;
    sections.sections = {make_sample_section()};

    auto payload = serde::serialize_sections(sections);
    auto sections2 = serde::deserialize_sections(payload);

    CHECK(sections2 == sections);
}

TEST_CASE("sections serde: optional fields absent round-trip as nullopt") {
    model::Section s;
    s.section_id = "sec-minimal";
    // headers/footers default-constructed (all nullopt), page_number_restart unset, content empty

    model::Sections sections;
    sections.sections = {s};

    auto payload = serde::serialize_sections(sections);
    auto sections2 = serde::deserialize_sections(payload);

    REQUIRE(sections2.sections.size() == 1);
    const auto& s2 = sections2.sections[0];
    CHECK_FALSE(s2.headers.default_.has_value());
    CHECK_FALSE(s2.headers.first.has_value());
    CHECK_FALSE(s2.headers.even.has_value());
    CHECK_FALSE(s2.page_number_restart.has_value());
    CHECK(s2.content.empty());
    CHECK(s2 == s);
}

TEST_CASE("sections serde: page_setup defaults round-trip") {
    model::Section s;
    s.section_id = "sec-defaults";
    // page_setup left at struct defaults: US-style default not assumed, just zeros/true/false as declared

    model::Sections sections;
    sections.sections = {s};

    auto payload = serde::serialize_sections(sections);
    auto sections2 = serde::deserialize_sections(payload);

    REQUIRE(sections2.sections.size() == 1);
    CHECK(sections2.sections[0].page_setup == s.page_setup);
    CHECK(sections2.sections[0].page_setup.orientation == model::Orientation::kPortrait);
    CHECK(sections2.sections[0].page_setup.columns.count == 1);
}

TEST_CASE("sections serde: negative margins round-trip correctly") {
    model::Section s;
    s.section_id = "sec-negative-margins";
    s.page_setup.margins = {-100, -50, 0, 200, 10};

    model::Sections sections;
    sections.sections = {s};

    auto payload = serde::serialize_sections(sections);
    auto sections2 = serde::deserialize_sections(payload);

    REQUIRE(sections2.sections.size() == 1);
    CHECK(sections2.sections[0].page_setup.margins.top == -100);
    CHECK(sections2.sections[0].page_setup.margins.bottom == -50);
    CHECK(sections2.sections[0].page_setup.margins.right == 200);
}

TEST_CASE("sections serde: multiple sections preserve order") {
    model::Section s1;
    s1.section_id = "sec-1";
    model::Section s2;
    s2.section_id = "sec-2";
    model::Section s3;
    s3.section_id = "sec-3";

    model::Sections sections;
    sections.sections = {s1, s2, s3};

    auto payload = serde::serialize_sections(sections);
    auto sections2 = serde::deserialize_sections(payload);

    REQUIRE(sections2.sections.size() == 3);
    CHECK(sections2.sections[0].section_id == "sec-1");
    CHECK(sections2.sections[1].section_id == "sec-2");
    CHECK(sections2.sections[2].section_id == "sec-3");
}

TEST_CASE("sections serde: empty sections list round-trips") {
    model::Sections sections;
    auto payload = serde::serialize_sections(sections);
    auto sections2 = serde::deserialize_sections(payload);
    CHECK(sections2.sections.empty());
}

TEST_CASE("sections serde: unknown field within a Section is skipped") {
    model::Sections sections;
    sections.sections = {make_sample_section()};
    auto payload = serde::serialize_sections(sections);

    auto outer_records = tlv::parse_records(payload);
    REQUIRE(outer_records.size() == 1);
    std::vector<uint8_t> patched_inner = outer_records[0].payload;
    tlv::write_record(patched_inner, 999, 1, {0x01}); // future field this reader doesn't know

    std::vector<uint8_t> patched_payload;
    tlv::write_record(patched_payload, outer_records[0].header.type_id,
                       outer_records[0].header.schema_version, patched_inner);

    auto sections2 = serde::deserialize_sections(patched_payload);
    REQUIRE(sections2.sections.size() == 1);
    CHECK(sections2.sections[0].section_id == "sec-1");
}

TEST_CASE("sections serde: unknown top-level record type is skipped, not fatal") {
    model::Sections sections;
    auto payload = serde::serialize_sections(sections);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto sections2 = serde::deserialize_sections(payload);
    CHECK(sections2.sections.empty());
}
