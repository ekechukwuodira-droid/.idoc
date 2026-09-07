#pragma once
// Footnotes, Endnotes, Comments, Bookmarks, Hyperlinks — §10. Pure data,
// no I/O. Physically three separate container blocks per §1.1's layout
// (Footnotes/Endnotes, Comments, Bookmarks/Hyperlinks), modeled here as
// three separate top-level container structs at the bottom of this file.
//
// RESOLVED: `Note.content` and `Comment.content` (both `Block[]`) now use
// the reference format decided in model/content_ref.hpp -- the same one
// Section/Cell/PageGeometry content use.
//
// ASSUMPTION FLAGGED: `Note.number_format` reuses §7's `NumberFormat`
// as-is (no redefinition given here, unlike §8 which explicitly
// introduced a different closed set) -- so this is NOT treated as a
// naming collision; it's the same enum reused, which is what the spec
// appears to intend.
//
// ASSUMPTION FLAGGED (naming collision #3): `Note.restart_rule` is typed
// "RestartRule" with values "Continuous | PerPage | PerSection" -- a
// THIRD, different closed set reusing a name already used twice before
// (§7's level-based RestartRule with a level_index parameter; §5's
// invented NumberingRestart for page numbers). PerPage/PerSection have no
// equivalent in either prior set, so this gets its own distinct type,
// `NoteRestartRule`, rather than overloading `model::RestartRule` (§7)
// with values it was never designed to hold.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "idoc/model/content_ref.hpp" // ContentRef
#include "idoc/model/numbering.hpp"   // NumberFormat (reused as-is, see note above)

namespace idoc::model {

enum class NoteRestartRule : uint8_t {
    kContinuous = 0,
    kPerPage = 1,
    kPerSection = 2,
};

struct Note {
    std::string note_id;
    // Block[] -- see model/content_ref.hpp.
    std::vector<ContentRef> content;
    std::optional<NumberFormat> number_format; // independent numbering style per note type
    NoteRestartRule restart_rule = NoteRestartRule::kContinuous;

    bool operator==(const Note& other) const {
        return note_id == other.note_id && content == other.content &&
               number_format == other.number_format && restart_rule == other.restart_rule;
    }
};

struct Comment {
    std::string comment_id;
    std::string author;
    std::string created_at; // ISO 8601
    // Block[] -- see model/content_ref.hpp.
    std::vector<ContentRef> content;
    std::string anchor_run_id;                    // start
    std::optional<std::string> anchor_end_run_id; // end, if the range spans multiple runs
    std::optional<std::string> parent_comment_id; // for threaded replies
    bool resolved = false;

    bool operator==(const Comment& other) const {
        return comment_id == other.comment_id && author == other.author &&
               created_at == other.created_at && content == other.content &&
               anchor_run_id == other.anchor_run_id &&
               anchor_end_run_id == other.anchor_end_run_id &&
               parent_comment_id == other.parent_comment_id && resolved == other.resolved;
    }
};

struct Bookmark {
    std::string bookmark_id;
    std::string name;
    std::string start_run_id;
    std::string end_run_id;

    bool operator==(const Bookmark& other) const {
        return bookmark_id == other.bookmark_id && name == other.name &&
               start_run_id == other.start_run_id && end_run_id == other.end_run_id;
    }
};

struct Hyperlink {
    std::string hyperlink_id;
    std::string target; // URL, or "#bookmark_id" for internal links
    std::optional<std::string> tooltip;

    bool operator==(const Hyperlink& other) const {
        return hyperlink_id == other.hyperlink_id && target == other.target &&
               tooltip == other.tooltip;
    }
};

// Physical container layout's "Block 8: Footnotes/Endnotes".
struct FootnotesEndnotes {
    std::vector<Note> footnotes;
    std::vector<Note> endnotes;

    bool operator==(const FootnotesEndnotes& other) const {
        return footnotes == other.footnotes && endnotes == other.endnotes;
    }
};

// Physical container layout's "Block 9: Comments".
struct Comments {
    std::vector<Comment> comments;

    bool operator==(const Comments& other) const {
        return comments == other.comments;
    }
};

// Physical container layout's "Block 10: Bookmarks/Hyperlinks".
struct BookmarksHyperlinks {
    std::vector<Bookmark> bookmarks;
    std::vector<Hyperlink> hyperlinks;

    bool operator==(const BookmarksHyperlinks& other) const {
        return bookmarks == other.bookmarks && hyperlinks == other.hyperlinks;
    }
};

} // namespace idoc::model
