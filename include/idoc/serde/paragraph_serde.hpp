#pragma once
// DocumentContent (Paragraphs & Runs) block serde.
//
// ARCHITECTURE NOTE: the spec's §2 document tree types `Section.content`
// as `Block[]` (Paragraph | Table) embedded directly inside the Section.
// But §1.1's physical container layout separately numbers "Block 5:
// Document Content (paragraphs)" as its own top-level block, distinct
// from "Block 3: Sections". The spec doesn't reconcile these two views --
// this is a genuine open question, not something we're silently
// resolving.
//
// We've built Document Content as its own block (matching what the
// physical layout table explicitly names it), containing a flat,
// ID-addressable list of Paragraph records. The rationale: §1.6's
// incremental-save story (append changed blocks without rewriting
// others) only works at *paragraph* granularity if paragraphs are their
// own records rather than buried inside a monolithic Sections blob --
// editing one paragraph shouldn't require rewriting every Section.
//
// What this means going forward: `Section.content_raw` (reserved since
// the Sections stage) will eventually hold an ordered list of
// {content_type, content_id} references into this block (and into
// Tables, §9, once that exists) rather than embedded objects. That
// wiring is NOT built yet -- Section.content_raw stays an opaque
// reserved blob until that connection is made. Flagging this now so it
// doesn't get silently decided one way in a future stage.

#include "idoc/model/paragraph.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kParagraphsSchemaVersion = 1;

std::vector<uint8_t> serialize_document_content(const model::DocumentContent& dc);

// Throws std::out_of_range on truncated/malformed input.
model::DocumentContent deserialize_document_content(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
