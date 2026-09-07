#include <doctest/doctest.h>
#include "idoc/serde/theme_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::Theme make_sample_theme() {
    model::Theme t;
    t.colors.accent1 = {0x4A, 0x90, 0xD9, 255};
    t.colors.accent2 = {0xE8, 0x7A, 0x41, 255};
    t.colors.accent3 = {0x50, 0xC8, 0x78, 255};
    t.colors.accent4 = {0xC9, 0x4A, 0x4A, 255};
    t.colors.accent5 = {0x9B, 0x59, 0xB6, 255};
    t.colors.accent6 = {0xF1, 0xC4, 0x0F, 255};
    t.colors.text1 = {0x00, 0x00, 0x00, 255};
    t.colors.text2 = {0x40, 0x40, 0x40, 255};
    t.colors.background1 = {0xFF, 0xFF, 0xFF, 255};
    t.colors.background2 = {0xF5, 0xF5, 0xF5, 255};
    t.colors.hyperlink = {0x05, 0x63, 0xC1, 255};
    t.colors.followed_hyperlink = {0x95, 0x4F, 0x9C, 255};

    t.fonts.heading_major = {"Calibri Light", std::nullopt};
    t.fonts.heading_minor = {"Calibri Light", "Arial"};
    t.fonts.body = {"Calibri", "Arial"};

    return t;
}

} // namespace

TEST_CASE("theme serde: full round-trip") {
    model::Theme t = make_sample_theme();
    auto payload = serde::serialize_theme(t);
    auto t2 = serde::deserialize_theme(payload);
    CHECK(t2 == t);
}

TEST_CASE("theme serde: FontRef without fallback round-trips as nullopt") {
    model::Theme t = make_sample_theme();
    auto payload = serde::serialize_theme(t);
    auto t2 = serde::deserialize_theme(payload);
    CHECK_FALSE(t2.fonts.heading_major.fallback.has_value());
    REQUIRE(t2.fonts.heading_minor.fallback.has_value());
    CHECK(*t2.fonts.heading_minor.fallback == "Arial");
}

TEST_CASE("theme serde: default-constructed theme round-trips") {
    model::Theme t; // all colors default to {0,0,0,255}, fonts empty strings
    auto payload = serde::serialize_theme(t);
    auto t2 = serde::deserialize_theme(payload);
    CHECK(t2 == t);
}

TEST_CASE("theme serde: unknown top-level and nested fields are skipped") {
    model::Theme t = make_sample_theme();
    auto payload = serde::serialize_theme(t);

    // Simulate a future minor version adding a new top-level Theme field.
    tlv::write_record(payload, 999, 1, {0xAA});

    auto t2 = serde::deserialize_theme(payload);
    CHECK(t2 == t); // everything we do know about is unaffected
}
