#pragma once
// Theme — §4.1. Pure data, no I/O.
//
// ASSUMPTION FLAGGED: the spec defines `Color` and `FontRef` as types used
// throughout (Theme, RunProperties, etc.) but never gives their wire shape
// -- unlike BorderSet/Shading/TabStop[], which are at least *named* as
// deferred in the spec's "Open Questions" section, these aren't mentioned
// there at all, so this isn't a spec gap the authors already know about.
// We're making a minimal, reasonable choice and flagging it:
//   - Color: RGBA, one byte per channel (0-255), alpha 255 = opaque.
//   - FontRef: a font family name plus an optional fallback family.
// Both are easy to extend (e.g. Color -> add a named/theme-relative
// variant) without changing anything that already round-trips, since new
// fields append rather than replace.

#include <cstdint>
#include <optional>
#include <string>

namespace idoc::model {

struct Color {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

struct FontRef {
    std::string family;
    std::optional<std::string> fallback;

    bool operator==(const FontRef& other) const {
        return family == other.family && fallback == other.fallback;
    }
};

struct ColorScheme {
    Color accent1, accent2, accent3, accent4, accent5, accent6;
    Color text1, text2;
    Color background1, background2;
    Color hyperlink;
    Color followed_hyperlink;

    bool operator==(const ColorScheme& other) const {
        return accent1 == other.accent1 && accent2 == other.accent2 &&
               accent3 == other.accent3 && accent4 == other.accent4 &&
               accent5 == other.accent5 && accent6 == other.accent6 &&
               text1 == other.text1 && text2 == other.text2 &&
               background1 == other.background1 && background2 == other.background2 &&
               hyperlink == other.hyperlink && followed_hyperlink == other.followed_hyperlink;
    }
};

struct FontScheme {
    FontRef heading_major;
    FontRef heading_minor;
    FontRef body;

    bool operator==(const FontScheme& other) const {
        return heading_major == other.heading_major &&
               heading_minor == other.heading_minor &&
               body == other.body;
    }
};

struct Theme {
    ColorScheme colors;
    FontScheme fonts;

    bool operator==(const Theme& other) const {
        return colors == other.colors && fonts == other.fonts;
    }
};

} // namespace idoc::model
