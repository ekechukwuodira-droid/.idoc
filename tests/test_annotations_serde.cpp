#include <doctest/doctest.h>
#include "idoc/serde/annotations_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

TEST_CASE("annotations serde: footnote and endnote round-trip in their own lists") {
    model::Note footnote;
    footnote.note_id = "fn-1";
    footnote.content = {model::ContentRef{model::ContentType::kParagraph, "para-1"}};
    footnote.number_format = model::NumberFormat::kLowerRoman;
    footnote.restart_rule = model::NoteRestartRule::kPerPage;

    model::Note endnote;
    endnote.note_id = "en-1";
    endnote.restart_rule = model::NoteRestartRule::kContinuous;

    model::FootnotesEndnotes fe;
    fe.footnotes = {footnote};
    fe.endnotes = {endnote};

    auto payload = serde::serialize_footnotes_endnotes(fe);
    auto fe2 = serde::deserialize_footnotes_endnotes(payload);

    CHECK(fe2 == fe);
    REQUIRE(fe2.footnotes.size() == 1);
    REQUIRE(fe2.endnotes.size() == 1);
    CHECK(fe2.footnotes[0].note_id == "fn-1");
    CHECK(fe2.endnotes[0].note_id == "en-1");
}

TEST_CASE("annotations serde: note_id doesn't collide between footnote and endnote lists") {
    model::Note note_a;
    note_a.note_id = "shared-id";
    model::Note note_b;
    note_b.note_id = "shared-id"; // same ID, different list -- must not merge

    model::FootnotesEndnotes fe;
    fe.footnotes = {note_a};
    fe.endnotes = {note_b};

    auto payload = serde::serialize_footnotes_endnotes(fe);
    auto fe2 = serde::deserialize_footnotes_endnotes(payload);

    CHECK(fe2.footnotes.size() == 1);
    CHECK(fe2.endnotes.size() == 1);
}

TEST_CASE("annotations serde: note_format absent and content empty round-trip") {
    model::Note note;
    note.note_id = "fn-minimal";

    model::FootnotesEndnotes fe;
    fe.footnotes = {note};

    auto payload = serde::serialize_footnotes_endnotes(fe);
    auto fe2 = serde::deserialize_footnotes_endnotes(payload);

    CHECK_FALSE(fe2.footnotes[0].number_format.has_value());
    CHECK(fe2.footnotes[0].content.empty());
}

TEST_CASE("annotations serde: comment with full threading fields round-trips") {
    model::Comment parent;
    parent.comment_id = "c1";
    parent.author = "Mystic";
    parent.created_at = "2026-08-30T12:00:00Z";
    parent.anchor_run_id = "run-1";
    parent.anchor_end_run_id = "run-3";
    parent.resolved = false;

    model::Comment reply;
    reply.comment_id = "c2";
    reply.author = "Reviewer";
    reply.created_at = "2026-08-30T13:00:00Z";
    reply.anchor_run_id = "run-1";
    reply.parent_comment_id = "c1";
    reply.resolved = true;

    model::Comments comments;
    comments.comments = {parent, reply};

    auto payload = serde::serialize_comments(comments);
    auto comments2 = serde::deserialize_comments(payload);

    CHECK(comments2 == comments);
    REQUIRE(comments2.comments.size() == 2);
    CHECK(comments2.comments[1].parent_comment_id.value() == "c1");
    CHECK(comments2.comments[1].resolved == true);
}

TEST_CASE("annotations serde: comment optional fields absent round-trip as nullopt") {
    model::Comment c;
    c.comment_id = "c-minimal";
    c.author = "Someone";
    c.created_at = "2026-01-01T00:00:00Z";
    c.anchor_run_id = "run-1";
    // anchor_end_run_id, parent_comment_id left unset; content left empty

    model::Comments comments;
    comments.comments = {c};

    auto payload = serde::serialize_comments(comments);
    auto comments2 = serde::deserialize_comments(payload);

    CHECK_FALSE(comments2.comments[0].anchor_end_run_id.has_value());
    CHECK_FALSE(comments2.comments[0].parent_comment_id.has_value());
    CHECK(comments2.comments[0].content.empty());
}

TEST_CASE("annotations serde: bookmark and hyperlink round-trip in their own lists") {
    model::Bookmark b;
    b.bookmark_id = "bm-1";
    b.name = "chapter-two";
    b.start_run_id = "run-5";
    b.end_run_id = "run-5";

    model::Hyperlink h;
    h.hyperlink_id = "hl-1";
    h.target = "https://example.com";
    h.tooltip = "Visit example.com";

    model::BookmarksHyperlinks bh;
    bh.bookmarks = {b};
    bh.hyperlinks = {h};

    auto payload = serde::serialize_bookmarks_hyperlinks(bh);
    auto bh2 = serde::deserialize_bookmarks_hyperlinks(payload);

    CHECK(bh2 == bh);
}

TEST_CASE("annotations serde: internal hyperlink target (#bookmark_id) round-trips as plain string") {
    model::Hyperlink h;
    h.hyperlink_id = "hl-internal";
    h.target = "#chapter-two";
    // tooltip left unset

    model::BookmarksHyperlinks bh;
    bh.hyperlinks = {h};

    auto payload = serde::serialize_bookmarks_hyperlinks(bh);
    auto bh2 = serde::deserialize_bookmarks_hyperlinks(payload);

    REQUIRE(bh2.hyperlinks.size() == 1);
    CHECK(bh2.hyperlinks[0].target == "#chapter-two");
    CHECK_FALSE(bh2.hyperlinks[0].tooltip.has_value());
}

TEST_CASE("annotations serde: empty lists round-trip for all three blocks") {
    auto fe_payload = serde::serialize_footnotes_endnotes(model::FootnotesEndnotes{});
    CHECK(serde::deserialize_footnotes_endnotes(fe_payload).footnotes.empty());

    auto c_payload = serde::serialize_comments(model::Comments{});
    CHECK(serde::deserialize_comments(c_payload).comments.empty());

    auto bh_payload = serde::serialize_bookmarks_hyperlinks(model::BookmarksHyperlinks{});
    auto bh2 = serde::deserialize_bookmarks_hyperlinks(bh_payload);
    CHECK(bh2.bookmarks.empty());
    CHECK(bh2.hyperlinks.empty());
}

TEST_CASE("annotations serde: unknown top-level record types are skipped in all three blocks") {
    auto fe_payload = serde::serialize_footnotes_endnotes(model::FootnotesEndnotes{});
    tlv::write_record(fe_payload, 999, 1, {0xFF});
    CHECK(serde::deserialize_footnotes_endnotes(fe_payload).footnotes.empty());

    auto c_payload = serde::serialize_comments(model::Comments{});
    tlv::write_record(c_payload, 999, 1, {0xFF});
    CHECK(serde::deserialize_comments(c_payload).comments.empty());

    auto bh_payload = serde::serialize_bookmarks_hyperlinks(model::BookmarksHyperlinks{});
    tlv::write_record(bh_payload, 999, 1, {0xFF});
    CHECK(serde::deserialize_bookmarks_hyperlinks(bh_payload).bookmarks.empty());
}
