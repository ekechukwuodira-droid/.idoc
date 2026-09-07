#include <doctest/doctest.h>
#include "idoc/serde/layout_cache_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::LayoutCache make_sample_layout_cache() {
    model::LayoutCache lc;
    lc.generation = 7;

    model::PageGeometry page1;
    page1.page_number = 1;
    page1.section_id = "sec-1";
    page1.field_ids = {"field-page-number", "field-total-pages"};

    model::PageGeometry page2;
    page2.page_number = 2;
    page2.section_id = "sec-1";
    page2.content_block_refs_raw = std::vector<uint8_t>{0x01, 0x02, 0x03};
    page2.field_ids = {"field-page-number"};

    lc.pages = {page1, page2};
    lc.field_results["field-page-number"] = "2";
    lc.field_results["field-total-pages"] = "42";

    return lc;
}

} // namespace

TEST_CASE("layout_cache serde: full round-trip") {
    model::LayoutCache lc = make_sample_layout_cache();

    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);

    CHECK(lc2 == lc);
}

TEST_CASE("layout_cache serde: generation counter round-trips exactly") {
    model::LayoutCache lc = make_sample_layout_cache();
    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);
    CHECK(lc2.generation == 7);
}

TEST_CASE("layout_cache serde: field_results map round-trips all entries") {
    model::LayoutCache lc = make_sample_layout_cache();
    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);

    REQUIRE(lc2.field_results.size() == 2);
    CHECK(lc2.field_results.at("field-page-number") == "2");
    CHECK(lc2.field_results.at("field-total-pages") == "42");
}

TEST_CASE("layout_cache serde: pages preserve order and their own field_ids lists") {
    model::LayoutCache lc = make_sample_layout_cache();
    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);

    REQUIRE(lc2.pages.size() == 2);
    CHECK(lc2.pages[0].page_number == 1);
    CHECK(lc2.pages[1].page_number == 2);
    REQUIRE(lc2.pages[0].field_ids.size() == 2);
    CHECK(lc2.pages[0].field_ids[0] == "field-page-number");
    REQUIRE(lc2.pages[1].field_ids.size() == 1);
}

TEST_CASE("layout_cache serde: content_block_refs_raw absent round-trips as nullopt") {
    model::LayoutCache lc = make_sample_layout_cache();
    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);

    CHECK_FALSE(lc2.pages[0].content_block_refs_raw.has_value());
    REQUIRE(lc2.pages[1].content_block_refs_raw.has_value());
    CHECK(lc2.pages[1].content_block_refs_raw.value() == std::vector<uint8_t>{0x01, 0x02, 0x03});
}

TEST_CASE("layout_cache serde: empty cache (no pages, no field_results) round-trips") {
    model::LayoutCache lc;
    lc.generation = 0;

    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);

    CHECK(lc2.generation == 0);
    CHECK(lc2.pages.empty());
    CHECK(lc2.field_results.empty());
}

TEST_CASE("layout_cache serde: page with no field_ids round-trips with an empty list") {
    model::PageGeometry page;
    page.page_number = 1;
    page.section_id = "sec-1";
    // field_ids deliberately left empty

    model::LayoutCache lc;
    lc.pages = {page};

    auto payload = serde::serialize_layout_cache(lc);
    auto lc2 = serde::deserialize_layout_cache(payload);

    REQUIRE(lc2.pages.size() == 1);
    CHECK(lc2.pages[0].field_ids.empty());
}

TEST_CASE("layout_cache serde: unknown field within a page is skipped") {
    model::LayoutCache lc = make_sample_layout_cache();
    auto payload = serde::serialize_layout_cache(lc);

    auto records = tlv::parse_records(payload);
    // Find the first "page" record (top_field::kPage == 2) and patch its inner payload.
    std::vector<uint8_t> patched_payload;
    bool patched_one = false;
    for (const auto& rec : records) {
        if (!patched_one && rec.header.type_id == 2) {
            std::vector<uint8_t> patched_inner = rec.payload;
            tlv::write_record(patched_inner, 999, 1, {0x01});
            tlv::write_record(patched_payload, rec.header.type_id, rec.header.schema_version, patched_inner);
            patched_one = true;
        } else {
            tlv::write_record(patched_payload, rec.header.type_id, rec.header.schema_version, rec.payload);
        }
    }
    REQUIRE(patched_one);

    auto lc2 = serde::deserialize_layout_cache(patched_payload);
    REQUIRE(lc2.pages.size() == 2);
    CHECK(lc2.pages[0].section_id == "sec-1");
}

TEST_CASE("layout_cache serde: unknown top-level field is skipped, not fatal") {
    model::LayoutCache lc = make_sample_layout_cache();
    auto payload = serde::serialize_layout_cache(lc);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto lc2 = serde::deserialize_layout_cache(payload);
    CHECK(lc2 == lc);
}
