#pragma once
// Fields — §8, including the Page Number Designer (§8.1). Pure data, no I/O.
//
// SCOPE: only PageNumberField's payload is fully specified by the spec.
// §8.2 explicitly defers TableOfContents/CrossReference/StyleRef payload
// shapes to a follow-up doc ("shouldn't be finalized until [the layout
// engine's] resolution algorithm exists"). The remaining field types
// (TotalPages, ChapterNumber, SectionNumber, Date, Custom) aren't
// discussed at all. Rather than inventing structure for any of these,
// `Field` carries a fully-modeled `page_number_payload` only for
// FieldType::kPageNumber, and an opaque `other_payload_raw` for every
// other type -- consistent with how this codebase treats every other
// spec gap (BorderSet, Shading, TabStop[], etc.): reserve the slot,
// don't guess the shape.
//
// ASSUMPTION FLAGGED: `NumberFormat` is reused as a name in §8.1 for a
// *different* closed set than §7's `NumberFormat` (this one has
// `LeadingZeros(width)` and no `Bullet`/`Ordinal`/`ChineseCounting`/
// `CustomGlyph`/`Image`). Named `PageNumberFormat` here to avoid
// collision with model/numbering.hpp's `NumberFormat`.
//
// ASSUMPTION FLAGGED: "Custom Anchor anchor?" for an explicit x/y when
// position == kCustom doesn't state units -- modeled as twips (int32),
// consistent with every other measurement in this spec (page size,
// margins, indents).

#include "idoc/model/numbering.hpp"  // RestartRule (explicitly shared per the spec)
#include "idoc/model/paragraph.hpp" // RunProperties

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

enum class FieldType : uint8_t {
    kPageNumber = 0,
    kTotalPages = 1,
    kChapterNumber = 2,
    kSectionNumber = 3,
    kDate = 4,
    kCrossReference = 5,
    kTableOfContents = 6,
    kStyleRef = 7,
    kCustom = 8,
};

enum class PageNumberPosition : uint8_t {
    kHeader = 0,
    kFooter = 1,
    kMargin = 2,
    kInside = 3,
    kOutside = 4,
    kCustom = 5,
};

struct CustomAnchor {
    int32_t x = 0; // twips -- see ASSUMPTION FLAGGED note above
    int32_t y = 0;

    bool operator==(const CustomAnchor& other) const {
        return x == other.x && y == other.y;
    }
};

// See ASSUMPTION FLAGGED note above re: naming collision with §7's NumberFormat.
struct PageNumberFormat {
    enum class Kind : uint8_t {
        kDecimal = 0,
        kUpperRoman = 1,
        kLowerRoman = 2,
        kUpperLetter = 3,
        kLowerLetter = 4,
        kLeadingZeros = 5,
    };

    Kind kind = Kind::kDecimal;
    std::optional<uint32_t> width; // only meaningful when kind == kLeadingZeros

    bool operator==(const PageNumberFormat& other) const {
        return kind == other.kind && width == other.width;
    }
};

struct PageNumberFieldPayload {
    PageNumberPosition position = PageNumberPosition::kFooter;
    std::optional<CustomAnchor> anchor; // present iff position == kCustom
    PageNumberFormat format;
    bool include_total_pages = false;
    bool include_chapter_number = false;
    bool include_section_number = false;
    RestartRule restart_rule; // shared with §7 per the spec
    bool different_first_page = false;
    bool different_odd_even = false;
    std::optional<std::string> prefix;
    std::optional<std::string> suffix;
    std::string pattern; // e.g. "[Chapter] — [Section] — [Page] / [Total]"
    std::optional<RunProperties> number_style;

    bool operator==(const PageNumberFieldPayload& other) const {
        return position == other.position && anchor == other.anchor &&
               format == other.format && include_total_pages == other.include_total_pages &&
               include_chapter_number == other.include_chapter_number &&
               include_section_number == other.include_section_number &&
               restart_rule == other.restart_rule &&
               different_first_page == other.different_first_page &&
               different_odd_even == other.different_odd_even &&
               prefix == other.prefix && suffix == other.suffix && pattern == other.pattern &&
               number_style == other.number_style;
    }
};

struct Field {
    std::string field_id;
    FieldType type = FieldType::kPageNumber;

    // Populated iff type == kPageNumber; see SCOPE note above.
    std::optional<PageNumberFieldPayload> page_number_payload;
    // Reserved for every other FieldType until their payload shapes are
    // specified (§8.2 -- deferred pending the layout engine's multi-pass
    // resolution algorithm).
    std::optional<std::vector<uint8_t>> other_payload_raw;

    std::string cached_result; // last-computed display text -- always present (§0 principle #4)
    std::string cached_at;     // ISO 8601
    bool locked = false;

    bool operator==(const Field& other) const {
        return field_id == other.field_id && type == other.type &&
               page_number_payload == other.page_number_payload &&
               other_payload_raw == other.other_payload_raw &&
               cached_result == other.cached_result && cached_at == other.cached_at &&
               locked == other.locked;
    }
};

// Physical container layout's "Block 7: Fields".
struct Fields {
    std::vector<Field> fields;

    bool operator==(const Fields& other) const {
        return fields == other.fields;
    }
};

} // namespace idoc::model
