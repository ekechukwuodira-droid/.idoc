#include "idoc/serde/annotations_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

namespace idoc::serde {

namespace fe_record_type {
constexpr uint32_t kFootnote = 1;
constexpr uint32_t kEndnote = 2;
} // namespace fe_record_type

namespace note_field {
constexpr uint32_t kNoteId = 1;
constexpr uint32_t kContent = 2; // Block[] -- see model/content_ref.hpp
constexpr uint32_t kNumberFormat = 3;
constexpr uint32_t kRestartRule = 4;
} // namespace note_field

namespace comment_record_type {
constexpr uint32_t kComment = 1;
} // namespace comment_record_type

namespace comment_field {
constexpr uint32_t kCommentId = 1;
constexpr uint32_t kAuthor = 2;
constexpr uint32_t kCreatedAt = 3;
constexpr uint32_t kContent = 4; // Block[] -- see model/content_ref.hpp
constexpr uint32_t kAnchorRunId = 5;
constexpr uint32_t kAnchorEndRunId = 6;
constexpr uint32_t kParentCommentId = 7;
constexpr uint32_t kResolved = 8;
} // namespace comment_field

namespace bh_record_type {
constexpr uint32_t kBookmark = 1;
constexpr uint32_t kHyperlink = 2;
} // namespace bh_record_type

namespace bookmark_field {
constexpr uint32_t kBookmarkId = 1;
constexpr uint32_t kName = 2;
constexpr uint32_t kStartRunId = 3;
constexpr uint32_t kEndRunId = 4;
} // namespace bookmark_field

namespace hyperlink_field {
constexpr uint32_t kHyperlinkId = 1;
constexpr uint32_t kTarget = 2;
constexpr uint32_t kTooltip = 3;
} // namespace hyperlink_field

namespace {

// --- Note (shared shape for Footnote and Endnote) ---

std::vector<uint8_t> encode_note(const model::Note& note) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, note.note_id);
        tlv::write_record(out, note_field::kNoteId, kAnnotationsSchemaVersion, p);
    }
    tlv::write_record(out, note_field::kContent, kAnnotationsSchemaVersion,
                       common::encode_content_refs(note.content));
    if (note.number_format.has_value()) {
        tlv::write_record(out, note_field::kNumberFormat, kAnnotationsSchemaVersion,
                           std::vector<uint8_t>{static_cast<uint8_t>(*note.number_format)});
    }
    tlv::write_record(out, note_field::kRestartRule, kAnnotationsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(note.restart_rule)});

    return out;
}

model::Note decode_note(const std::vector<uint8_t>& payload) {
    model::Note note;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case note_field::kNoteId:
                note.note_id = r.read_string();
                break;
            case note_field::kContent:
                note.content = common::decode_content_refs(rec.payload);
                break;
            case note_field::kNumberFormat:
                note.number_format = static_cast<model::NumberFormat>(r.read_u8());
                break;
            case note_field::kRestartRule:
                note.restart_rule = static_cast<model::NoteRestartRule>(r.read_u8());
                break;
            default:
                break; // unknown field: skip
        }
    }
    return note;
}

// --- Comment ---

std::vector<uint8_t> encode_comment(const model::Comment& comment) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, comment.comment_id);
        tlv::write_record(out, comment_field::kCommentId, kAnnotationsSchemaVersion, p);
    }
    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, comment.author);
        tlv::write_record(out, comment_field::kAuthor, kAnnotationsSchemaVersion, p);
    }
    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, comment.created_at);
        tlv::write_record(out, comment_field::kCreatedAt, kAnnotationsSchemaVersion, p);
    }
    tlv::write_record(out, comment_field::kContent, kAnnotationsSchemaVersion,
                       common::encode_content_refs(comment.content));
    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, comment.anchor_run_id);
        tlv::write_record(out, comment_field::kAnchorRunId, kAnnotationsSchemaVersion, p);
    }
    if (comment.anchor_end_run_id.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *comment.anchor_end_run_id);
        tlv::write_record(out, comment_field::kAnchorEndRunId, kAnnotationsSchemaVersion, p);
    }
    if (comment.parent_comment_id.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *comment.parent_comment_id);
        tlv::write_record(out, comment_field::kParentCommentId, kAnnotationsSchemaVersion, p);
    }
    tlv::write_record(out, comment_field::kResolved, kAnnotationsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(comment.resolved ? 1 : 0)});

    return out;
}

model::Comment decode_comment(const std::vector<uint8_t>& payload) {
    model::Comment comment;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case comment_field::kCommentId:
                comment.comment_id = r.read_string();
                break;
            case comment_field::kAuthor:
                comment.author = r.read_string();
                break;
            case comment_field::kCreatedAt:
                comment.created_at = r.read_string();
                break;
            case comment_field::kContent:
                comment.content = common::decode_content_refs(rec.payload);
                break;
            case comment_field::kAnchorRunId:
                comment.anchor_run_id = r.read_string();
                break;
            case comment_field::kAnchorEndRunId:
                comment.anchor_end_run_id = r.read_string();
                break;
            case comment_field::kParentCommentId:
                comment.parent_comment_id = r.read_string();
                break;
            case comment_field::kResolved:
                comment.resolved = r.read_u8() != 0;
                break;
            default:
                break; // unknown field: skip
        }
    }
    return comment;
}

// --- Bookmark / Hyperlink ---

std::vector<uint8_t> encode_bookmark(const model::Bookmark& b) {
    std::vector<uint8_t> out;
    tlv::write_record(out, bookmark_field::kBookmarkId, kAnnotationsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, b.bookmark_id); return p; }());
    tlv::write_record(out, bookmark_field::kName, kAnnotationsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, b.name); return p; }());
    tlv::write_record(out, bookmark_field::kStartRunId, kAnnotationsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, b.start_run_id); return p; }());
    tlv::write_record(out, bookmark_field::kEndRunId, kAnnotationsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, b.end_run_id); return p; }());
    return out;
}

model::Bookmark decode_bookmark(const std::vector<uint8_t>& payload) {
    model::Bookmark b;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case bookmark_field::kBookmarkId: b.bookmark_id = r.read_string(); break;
            case bookmark_field::kName: b.name = r.read_string(); break;
            case bookmark_field::kStartRunId: b.start_run_id = r.read_string(); break;
            case bookmark_field::kEndRunId: b.end_run_id = r.read_string(); break;
            default: break; // unknown field: skip
        }
    }
    return b;
}

std::vector<uint8_t> encode_hyperlink(const model::Hyperlink& h) {
    std::vector<uint8_t> out;
    tlv::write_record(out, hyperlink_field::kHyperlinkId, kAnnotationsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, h.hyperlink_id); return p; }());
    tlv::write_record(out, hyperlink_field::kTarget, kAnnotationsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, h.target); return p; }());
    if (h.tooltip.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *h.tooltip);
        tlv::write_record(out, hyperlink_field::kTooltip, kAnnotationsSchemaVersion, p);
    }
    return out;
}

model::Hyperlink decode_hyperlink(const std::vector<uint8_t>& payload) {
    model::Hyperlink h;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case hyperlink_field::kHyperlinkId: h.hyperlink_id = r.read_string(); break;
            case hyperlink_field::kTarget: h.target = r.read_string(); break;
            case hyperlink_field::kTooltip: h.tooltip = r.read_string(); break;
            default: break; // unknown field: skip
        }
    }
    return h;
}

} // namespace

std::vector<uint8_t> serialize_footnotes_endnotes(const model::FootnotesEndnotes& fe) {
    std::vector<uint8_t> out;
    for (const auto& n : fe.footnotes) {
        tlv::write_record(out, fe_record_type::kFootnote, kAnnotationsSchemaVersion, encode_note(n));
    }
    for (const auto& n : fe.endnotes) {
        tlv::write_record(out, fe_record_type::kEndnote, kAnnotationsSchemaVersion, encode_note(n));
    }
    return out;
}

model::FootnotesEndnotes deserialize_footnotes_endnotes(const std::vector<uint8_t>& payload) {
    model::FootnotesEndnotes fe;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id == fe_record_type::kFootnote) {
            fe.footnotes.push_back(decode_note(rec.payload));
        } else if (rec.header.type_id == fe_record_type::kEndnote) {
            fe.endnotes.push_back(decode_note(rec.payload));
        }
        // unknown top-level record type: skip
    }
    return fe;
}

std::vector<uint8_t> serialize_comments(const model::Comments& c) {
    std::vector<uint8_t> out;
    for (const auto& comment : c.comments) {
        tlv::write_record(out, comment_record_type::kComment, kAnnotationsSchemaVersion, encode_comment(comment));
    }
    return out;
}

model::Comments deserialize_comments(const std::vector<uint8_t>& payload) {
    model::Comments c;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != comment_record_type::kComment) continue; // unknown: skip
        c.comments.push_back(decode_comment(rec.payload));
    }
    return c;
}

std::vector<uint8_t> serialize_bookmarks_hyperlinks(const model::BookmarksHyperlinks& bh) {
    std::vector<uint8_t> out;
    for (const auto& b : bh.bookmarks) {
        tlv::write_record(out, bh_record_type::kBookmark, kAnnotationsSchemaVersion, encode_bookmark(b));
    }
    for (const auto& h : bh.hyperlinks) {
        tlv::write_record(out, bh_record_type::kHyperlink, kAnnotationsSchemaVersion, encode_hyperlink(h));
    }
    return out;
}

model::BookmarksHyperlinks deserialize_bookmarks_hyperlinks(const std::vector<uint8_t>& payload) {
    model::BookmarksHyperlinks bh;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id == bh_record_type::kBookmark) {
            bh.bookmarks.push_back(decode_bookmark(rec.payload));
        } else if (rec.header.type_id == bh_record_type::kHyperlink) {
            bh.hyperlinks.push_back(decode_hyperlink(rec.payload));
        }
        // unknown top-level record type: skip
    }
    return bh;
}

} // namespace idoc::serde
