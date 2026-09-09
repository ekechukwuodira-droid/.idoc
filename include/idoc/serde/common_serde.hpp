#pragma once
// Shared encode/decode for primitive types reused across more than one
// block's serde. Color/FontRef are used by Theme and by RunProperties.
// Alignment/Indent/Spacing/RunProperties are used by both Paragraphs
// (§6, direct formatting) and Numbering (§7, LevelDefinition's
// alignment/indent/number_run_props) -- pulling them out here means
// neither block's serde duplicates the other's encoding logic, and a
// future wire-shape change only needs updating in one place.

#include "idoc/model/content_ref.hpp"   // ContentType, ContentRef
#include "idoc/model/numbering.hpp"  // RestartRule
#include "idoc/model/paragraph.hpp" // Alignment, Indent, Spacing, RunProperties
#include "idoc/model/sections.hpp"  // Size2D
#include "idoc/model/theme.hpp"     // Color, FontRef

#include <cstdint>
#include <vector>

namespace idoc::serde::common {

std::vector<uint8_t> encode_color(const model::Color& c);
model::Color decode_color(const std::vector<uint8_t>& payload);

std::vector<uint8_t> encode_font_ref(const model::FontRef& f);
model::FontRef decode_font_ref(const std::vector<uint8_t>& payload);

std::vector<uint8_t> encode_indent(const model::Indent& i);
model::Indent decode_indent(const std::vector<uint8_t>& payload);

std::vector<uint8_t> encode_size2d(const model::Size2D& s);
model::Size2D decode_size2d(const std::vector<uint8_t>& payload);

std::vector<uint8_t> encode_spacing(const model::Spacing& s);
model::Spacing decode_spacing(const std::vector<uint8_t>& payload);

// RunProperties is a full nested-TLV field list (all fields optional), not
// a fixed-shape payload like the others above -- schema_version is needed
// for its internal field records.
std::vector<uint8_t> encode_run_properties(const model::RunProperties& rp, uint16_t schema_version);
model::RunProperties decode_run_properties(const std::vector<uint8_t>& payload);

// Same treatment for ParagraphProperties -- needed by both Paragraphs
// (§6, direct formatting) and Styles (§4, StyleDefinition.paragraph_props)
// once the style resolver needed real style-level properties to walk.
std::vector<uint8_t> encode_paragraph_properties(const model::ParagraphProperties& pp, uint16_t schema_version);
model::ParagraphProperties decode_paragraph_properties(const std::vector<uint8_t>& payload);

// Shared with §8's PageNumberFieldPayload.restart_rule ("shared enum with
// §7's numbering restart" per the spec).
std::vector<uint8_t> encode_restart_rule(const model::RestartRule& rr);
model::RestartRule decode_restart_rule(const std::vector<uint8_t>& payload);

// The Block[] reference format -- see model/content_ref.hpp. Used by
// Section.content, HeaderFooterContent.content, Cell.content,
// Note.content, Comment.content, and PageGeometry.content_block_refs.
// encode_content_refs/decode_content_refs handle the whole ordered list
// as a single self-delimiting TLV payload (a sequence of ref records);
// callers wrap that payload in their own field's TLV record as usual.
std::vector<uint8_t> encode_content_ref(const model::ContentRef& ref);
model::ContentRef decode_content_ref(const std::vector<uint8_t>& payload);

std::vector<uint8_t> encode_content_refs(const std::vector<model::ContentRef>& refs);
std::vector<model::ContentRef> decode_content_refs(const std::vector<uint8_t>& payload);

} // namespace idoc::serde::common

