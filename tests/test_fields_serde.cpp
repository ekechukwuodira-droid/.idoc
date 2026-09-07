#include <doctest/doctest.h>
#include "idoc/serde/fields_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::Field make_sample_page_number_field() {
    model::Field f;
    f.field_id = "field-1";
    f.type = model::FieldType::kPageNumber;

    model::PageNumberFieldPayload payload;
    payload.position = model::PageNumberPosition::kFooter;
    payload.format.kind = model::PageNumberFormat::Kind::kDecimal;
    payload.include_total_pages = true;
    payload.include_chapter_number = false;
    payload.restart_rule.kind = model::RestartRule::Kind::kRestartEachSection;
    payload.different_first_page = true;
    payload.prefix = "Page ";
    payload.suffix = std::nullopt;
    payload.pattern = "[Page] / [Total]";

    model::RunProperties style;
    style.bold = true;
    payload.number_style = style;

    f.page_number_payload = payload;
    f.cached_result = "Page 3 / 42";
    f.cached_at = "2026-08-30T12:00:00Z";
    f.locked = false;

    return f;
}

} // namespace

TEST_CASE("fields serde: full PageNumberField round-trip") {
    model::Fields fields;
    fields.fields = {make_sample_page_number_field()};

    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);

    CHECK(fields2 == fields);
}

TEST_CASE("fields serde: LeadingZeros format carries its width") {
    model::Field f = make_sample_page_number_field();
    f.page_number_payload->format.kind = model::PageNumberFormat::Kind::kLeadingZeros;
    f.page_number_payload->format.width = 3;

    model::Fields fields;
    fields.fields = {f};

    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);

    REQUIRE(fields2.fields.size() == 1);
    const auto& fmt = fields2.fields[0].page_number_payload->format;
    CHECK(fmt.kind == model::PageNumberFormat::Kind::kLeadingZeros);
    CHECK(fmt.width.value() == 3);
}

TEST_CASE("fields serde: custom position carries its anchor") {
    model::Field f = make_sample_page_number_field();
    f.page_number_payload->position = model::PageNumberPosition::kCustom;
    f.page_number_payload->anchor = model::CustomAnchor{1000, 2000};

    model::Fields fields;
    fields.fields = {f};

    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);

    REQUIRE(fields2.fields[0].page_number_payload->anchor.has_value());
    CHECK(fields2.fields[0].page_number_payload->anchor->x == 1000);
    CHECK(fields2.fields[0].page_number_payload->anchor->y == 2000);
}

TEST_CASE("fields serde: non-PageNumber field type uses opaque payload") {
    model::Field f;
    f.field_id = "field-2";
    f.type = model::FieldType::kDate;
    f.other_payload_raw = std::vector<uint8_t>{0x01, 0x02, 0x03};
    f.cached_result = "September 6, 2026";
    f.cached_at = "2026-09-06T00:00:00Z";
    f.locked = true;

    model::Fields fields;
    fields.fields = {f};

    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);

    REQUIRE(fields2.fields.size() == 1);
    CHECK_FALSE(fields2.fields[0].page_number_payload.has_value());
    CHECK(fields2.fields[0].other_payload_raw.value() == std::vector<uint8_t>{0x01, 0x02, 0x03});
    CHECK(fields2.fields[0].cached_result == "September 6, 2026");
    CHECK(fields2.fields[0].locked == true);
}

TEST_CASE("fields serde: cached_result is always present even without a full layout pass") {
    // Per §0 principle #4 -- degraded rendering reads cached_result directly.
    model::Field f;
    f.field_id = "field-3";
    f.type = model::FieldType::kTotalPages;
    f.cached_result = "12";
    f.cached_at = "2026-01-01T00:00:00Z";

    model::Fields fields;
    fields.fields = {f};

    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);

    CHECK(fields2.fields[0].cached_result == "12");
}

TEST_CASE("fields serde: prefix/suffix absent round-trip as nullopt") {
    model::Field f = make_sample_page_number_field();
    f.page_number_payload->prefix = std::nullopt;
    f.page_number_payload->suffix = std::nullopt;

    model::Fields fields;
    fields.fields = {f};

    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);

    CHECK_FALSE(fields2.fields[0].page_number_payload->prefix.has_value());
    CHECK_FALSE(fields2.fields[0].page_number_payload->suffix.has_value());
}

TEST_CASE("fields serde: empty Fields round-trips") {
    model::Fields fields;
    auto payload = serde::serialize_fields(fields);
    auto fields2 = serde::deserialize_fields(payload);
    CHECK(fields2.fields.empty());
}

TEST_CASE("fields serde: unknown top-level record type is skipped, not fatal") {
    model::Fields fields;
    fields.fields = {make_sample_page_number_field()};
    auto payload = serde::serialize_fields(fields);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto fields2 = serde::deserialize_fields(payload);
    REQUIRE(fields2.fields.size() == 1);
    CHECK(fields2.fields[0].field_id == "field-1");
}
