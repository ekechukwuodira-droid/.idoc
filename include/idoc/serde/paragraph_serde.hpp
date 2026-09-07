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
// What this means going forward: `Section.content` (and
// Cell.content/Note.content/Comment.content/
// PageGeometry.content_block_refs) now hold an ordered list of
// `ContentRef` values (see model/content_ref.hpp) pointing into this
// block by paragraph_id -- that reference format has been decided and
// implemented; see model/content_ref.hpp for the full reasoning.

#include "idoc/model/paragraph.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kParagraphsSchemaVersion = 1;

std::vector<uint8_t> serialize_document_content(const model::DocumentContent& dc);

// Throws std::out_of_range on truncated/malformed input.
model::DocumentContent deserialize_document_content(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
