#pragma once
// Style Resolver — walks the cascade the spec's document model overview
// describes: direct formatting -> style chain (based_on) -> doc default
// -> engine default. Pure logic over already-loaded model data; no I/O.
//
// This is a genuinely different layer from container/model/serde: those
// are about getting bytes in and out of a document faithfully. This is
// about answering "what does this paragraph/run actually look like,"
// which requires walking relationships between objects rather than just
// decoding one object's own fields.
//
// DESIGN: resolve_paragraph/resolve_run take the specific pieces they
// need (an optional style_id and optional direct-formatting struct)
// rather than full model::Paragraph/model::Run objects, so the resolver
// has no dependency on how those are stored or looked up -- it's a pure
// function of "what does this style_id reference, and what did this
// level override." Convenience overloads that take the full objects are
// provided below for ergonomic call sites.
//
// PRIORITY ORDER (paragraph): direct_props on the Paragraph itself, then
// its style chain (style_id -> based_on -> based_on -> ...), then the
// document's default paragraph style (StyleType::kParagraph with
// is_default == true) if not already in that chain, then engine
// defaults. First level to set a given property wins for that property
// specifically -- resolution is per-field, not per-object, matching how
// real word processors cascade formatting.
//
// PRIORITY ORDER (run): direct_props on the Run itself, then (if
// direct_props names a character style_id) that character style's
// chain, then the owning paragraph's style chain (a paragraph style also
// carries default character formatting for text typed in it), then the
// document's default character style if any, then engine defaults.
//
// CYCLE SAFETY: `based_on` chains are walked with a visited-set guard.
// A cycle in corrupt/hand-edited data stops the walk at the point of
// repetition rather than looping forever; it does not throw, since a
// corrupt style chain should degrade to "resolve with what's usable,"
// not crash resolution for the whole document.

#include "idoc/model/paragraph.hpp"
#include "idoc/model/styles.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace idoc::resolve {

// Concrete, fully-resolved paragraph formatting -- every field has a
// real value, never "not set." This is the resolver's output, as
// opposed to model::ParagraphProperties (every field optional), which
// represents what one level of the cascade overrides.
struct ResolvedParagraphFormat {
    model::Alignment alignment = model::Alignment::kLeft;
    model::Indent indent;
    model::Spacing spacing;
    bool keep_with_next = false;
    bool keep_lines_together = false;
    bool page_break_before = false;

    bool operator==(const ResolvedParagraphFormat& other) const {
        return alignment == other.alignment && indent == other.indent &&
               spacing == other.spacing && keep_with_next == other.keep_with_next &&
               keep_lines_together == other.keep_lines_together &&
               page_break_before == other.page_break_before;
    }
};

// Concrete, fully-resolved run (character) formatting. Engine defaults
// chosen to match common real-world word processor defaults (Calibri
// 11pt, black text, no highlight) -- these are the values used when
// nothing anywhere in the cascade sets a property.
struct ResolvedRunFormat {
    model::FontRef font = model::FontRef{"Calibri", std::string("Arial")};
    float size_pt = 11.0f;
    bool bold = false;
    bool italic = false;
    uint8_t underline = model::underline_value::kNone;
    bool strikethrough = false;
    model::TextVerticalAlign vertical_align = model::TextVerticalAlign::kBaseline;
    model::Color color = model::Color{0, 0, 0, 255};       // black
    model::Color highlight = model::Color{0, 0, 0, 0};     // fully transparent == no highlight
    float character_spacing_pt = 0.0f;
    bool small_caps = false;
    bool all_caps = false;

    bool operator==(const ResolvedRunFormat& other) const {
        return font == other.font && size_pt == other.size_pt && bold == other.bold &&
               italic == other.italic && underline == other.underline &&
               strikethrough == other.strikethrough && vertical_align == other.vertical_align &&
               color == other.color && highlight == other.highlight &&
               character_spacing_pt == other.character_spacing_pt &&
               small_caps == other.small_caps && all_caps == other.all_caps;
    }
};

class StyleResolver {
public:
    explicit StyleResolver(model::Styles styles);

    // Returns nullptr if no style with this id exists.
    const model::StyleDefinition* find_style(const std::string& style_id) const;

    // Returns the style flagged is_default for this StyleType, or
    // nullptr if none exists (a document need not define one).
    const model::StyleDefinition* find_default_style(model::StyleType type) const;

    ResolvedParagraphFormat resolve_paragraph(
        const std::optional<std::string>& style_id,
        const std::optional<model::ParagraphProperties>& direct_props) const;

    ResolvedRunFormat resolve_run(
        const std::optional<std::string>& paragraph_style_id,
        const std::optional<model::RunProperties>& run_direct_props) const;

    // Convenience overloads for the common case of resolving an actual
    // Paragraph/Run pair.
    ResolvedParagraphFormat resolve_paragraph(const model::Paragraph& paragraph) const {
        return resolve_paragraph(paragraph.style_id, paragraph.direct_props);
    }
    ResolvedRunFormat resolve_run(const model::Paragraph& paragraph, const model::Run& run) const {
        return resolve_run(paragraph.style_id, run.direct_props);
    }

private:
    model::Styles styles_; // owned; by_id_ points into styles_.definitions, so this must outlive it
    std::unordered_map<std::string, const model::StyleDefinition*> by_id_;

    // Walks based_on starting at style_id, nearest-first. Stops (without
    // throwing) on a cycle or a dangling reference. Returns an empty
    // vector if style_id itself doesn't resolve to anything.
    std::vector<const model::StyleDefinition*> resolve_chain(const std::string& style_id) const;
};

} // namespace idoc::resolve
