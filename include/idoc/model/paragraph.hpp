#pragma once
// Paragraphs & Runs — §6. Pure data, no I/O.
//
// ARCHITECTURE NOTE: `ParagraphProperties`' fields are all `optional<T>`,
// not plain `T` with a default value. This matters for the style
// resolver (resolve/style_resolver.hpp): resolution needs to distinguish
// "this level explicitly sets alignment to Left" from "this level says
// nothing about alignment, keep looking up the chain." A plain
// `Alignment alignment = kLeft` can't represent that distinction --
// every level would look like it explicitly chose Left. `RunProperties`
// already got this right from the start (below); this file originally
// didn't, and was corrected once the resolver needed real cascading
// semantics rather than just round-tripping a single level's values.
//
// DEFERRED (matching the spec's own "Open Questions" deferral of these
// exact three types as "low-risk, mechanical"): ParagraphProperties'
// `borders`, `shading`, and `tab_stops` are reserved-but-empty optional
// raw byte slots.
//
// ASSUMPTION FLAGGED: `Underline` is explicitly open-ended in the spec
// ("None | Single | Double | Word-only | Dotted...") -- the "..." means
// more values exist that aren't enumerated. Unlike a closed enum
// (Alignment, Orientation, StyleType), we do NOT want a reader to throw
// on an unrecognized numeric value here, since a future minor version is
// expected to add more. So `underline` is a raw optional byte with named
// constants for the values the spec does give us, not a strict enum --
// an unrecognized value round-trips untouched instead of failing to
// parse. See `underline_value` below.
//
// `VerticalAlign` here (Baseline | Superscript | Subscript, a text
// property) is a different concept from table cell vertical alignment
// (Top | Center | Bottom, §9) despite the spec using the same name for
// both -- named `TextVerticalAlign` here to avoid a collision once §9 is
// built.

#include "idoc/model/theme.hpp" // Color, FontRef

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

enum class Alignment : uint8_t {
    kLeft = 0,
    kRight = 1,
    kCenter = 2,
    kJustify = 3,
    kDistribute = 4,
};

struct Indent {
    int32_t left = 0;       // twips
    int32_t right = 0;
    int32_t first_line = 0;
    int32_t hanging = 0;

    bool operator==(const Indent& other) const {
        return left == other.left && right == other.right &&
               first_line == other.first_line && hanging == other.hanging;
    }
};

enum class LineRule : uint8_t {
    kSingle = 0,
    kOnePtFive = 1,
    kDouble = 2,
    kAtLeast = 3,
    kExact = 4,
    kMultiple = 5,
};

struct Spacing {
    uint32_t before = 0;  // twips
    uint32_t after = 0;   // twips
    int32_t line = 0;     // meaning depends on line_rule (twips, or 240ths for kMultiple)
    LineRule line_rule = LineRule::kSingle;

    bool operator==(const Spacing& other) const {
        return before == other.before && after == other.after &&
               line == other.line && line_rule == other.line_rule;
    }
};

struct ParagraphProperties {
    std::optional<Alignment> alignment;
    std::optional<Indent> indent;
    std::optional<Spacing> spacing;
    std::optional<bool> keep_with_next;
    std::optional<bool> keep_lines_together;
    std::optional<bool> page_break_before;

    // Reserved -- see DEFERRED note above.
    std::optional<std::vector<uint8_t>> borders_raw;
    std::optional<std::vector<uint8_t>> shading_raw;
    std::optional<std::vector<uint8_t>> tab_stops_raw;

    bool operator==(const ParagraphProperties& other) const {
        return alignment == other.alignment && indent == other.indent &&
               spacing == other.spacing && keep_with_next == other.keep_with_next &&
               keep_lines_together == other.keep_lines_together &&
               page_break_before == other.page_break_before &&
               borders_raw == other.borders_raw && shading_raw == other.shading_raw &&
               tab_stops_raw == other.tab_stops_raw;
    }
};

// See ASSUMPTION FLAGGED note above -- open-ended, not a closed enum.
namespace underline_value {
constexpr uint8_t kNone = 0;
constexpr uint8_t kSingle = 1;
constexpr uint8_t kDouble = 2;
constexpr uint8_t kWordOnly = 3;
constexpr uint8_t kDotted = 4;
} // namespace underline_value

enum class TextVerticalAlign : uint8_t {
    kBaseline = 0,
    kSuperscript = 1,
    kSubscript = 2,
};

struct RunProperties {
    std::optional<std::string> style_id;
    std::optional<FontRef> font;
    std::optional<float> size_pt;
    std::optional<bool> bold;
    std::optional<bool> italic;
    std::optional<uint8_t> underline; // see underline_value namespace above
    std::optional<bool> strikethrough;
    std::optional<TextVerticalAlign> vertical_align;
    std::optional<Color> color;
    std::optional<Color> highlight;
    std::optional<float> character_spacing_pt;
    std::optional<bool> small_caps;
    std::optional<bool> all_caps;

    bool operator==(const RunProperties& other) const {
        return style_id == other.style_id && font == other.font &&
               size_pt == other.size_pt && bold == other.bold && italic == other.italic &&
               underline == other.underline && strikethrough == other.strikethrough &&
               vertical_align == other.vertical_align && color == other.color &&
               highlight == other.highlight &&
               character_spacing_pt == other.character_spacing_pt &&
               small_caps == other.small_caps && all_caps == other.all_caps;
    }
};

// -> NumberingInstance + level (§7). Kept minimal (just the reference IDs)
// rather than requiring the full NumberingInstance object to exist here.
struct ListRef {
    std::string instance_id;
    int32_t level = 0;

    bool operator==(const ListRef& other) const {
        return instance_id == other.instance_id && level == other.level;
    }
};

// -> Field (§8), by stable ID -- the spec mentions "FieldRef" but never
// defines its shape beyond "if this run is a field result (see §8)", so
// this follows the same reference-by-ID minimalism as ListRef above.
struct FieldRef {
    std::string field_id;

    bool operator==(const FieldRef& other) const {
        return field_id == other.field_id;
    }
};

struct Run {
    std::string run_id;
    std::string text;
    std::optional<RunProperties> direct_props;

    // Was a reserved raw byte slot until §8 (Field) existed; now a real
    // reference. If this run is a field result, `text` is the cached
    // last-computed value (per §8's Field.cached_result contract).
    std::optional<FieldRef> field_ref;

    std::optional<std::string> hyperlink_id;
    std::optional<std::vector<std::string>> comment_anchor_ids;

    bool operator==(const Run& other) const {
        return run_id == other.run_id && text == other.text &&
               direct_props == other.direct_props && field_ref == other.field_ref &&
               hyperlink_id == other.hyperlink_id &&
               comment_anchor_ids == other.comment_anchor_ids;
    }
};

struct Paragraph {
    std::string paragraph_id;
    std::optional<std::string> style_id;
    std::optional<ParagraphProperties> direct_props;
    std::optional<ListRef> list_ref;
    std::vector<Run> runs;

    bool operator==(const Paragraph& other) const {
        return paragraph_id == other.paragraph_id && style_id == other.style_id &&
               direct_props == other.direct_props && list_ref == other.list_ref &&
               runs == other.runs;
    }
};

// Physical container layout's "Block 5: Document Content" — see the note
// in serde/paragraph_serde.hpp about how this relates to Section.content.
struct DocumentContent {
    std::vector<Paragraph> paragraphs;

    bool operator==(const DocumentContent& other) const {
        return paragraphs == other.paragraphs;
    }
};

} // namespace idoc::model
