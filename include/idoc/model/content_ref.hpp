#pragma once
// The reference format that Section.content, HeaderFooterContent.content,
// Cell.content, Note.content, Comment.content, and
// PageGeometry.content_block_refs all resolve to.
//
// RESOLUTION OF A LONG-STANDING OPEN QUESTION: §2's document tree types
// all of the above as `Block[]` (Paragraph | Table) embedded directly.
// But §1.1's physical container layout stores Paragraphs and Tables in
// their own separate top-level blocks (Document Content, Tables),
// addressable by paragraph_id/table_id. This file is the concrete
// resolution: an ordered list of {content_type, content_id} pairs that
// point into those blocks by ID, rather than embedding the objects
// themselves. This was flagged as an open architectural question across
// six separate reserved fields (see git history for the earlier stages
// that deferred each one); this is where it gets decided.
//
// WHY REFERENCES INSTEAD OF EMBEDDING: it's what makes §1.6's
// incremental-save story (append changed blocks without rewriting
// others) actually work at paragraph granularity. If Section.content
// embedded full Paragraph objects, editing one paragraph's text would
// mean rewriting the entire Sections block. With references,
// Section/Cell/Note/Comment content lists only change when content is
// added, removed, or reordered -- editing an existing paragraph's text
// only touches Document Content, never anything that references it by
// ID. The same reasoning that justified making Document Content its own
// block in the first place (serde/paragraph_serde.hpp) extends cleanly
// to every place that needs to point into it.
//
// A useful side effect of ID-based references rather than embedding:
// recursive nesting (a Table cell containing another Table) falls out
// for free -- Cell.content is just a list of ContentRefs, one of which
// can have type == kTable, no special-case needed.

#include <cstdint>
#include <string>
#include <vector>

namespace idoc::model {

enum class ContentType : uint8_t {
    kParagraph = 0, // -> Document Content block, by paragraph_id
    kTable = 1,     // -> Tables block, by table_id
};

struct ContentRef {
    ContentType type = ContentType::kParagraph;
    std::string content_id;

    bool operator==(const ContentRef& other) const {
        return type == other.type && content_id == other.content_id;
    }
};

} // namespace idoc::model
