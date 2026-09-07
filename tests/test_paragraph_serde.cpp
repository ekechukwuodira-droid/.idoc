#include <doctest/doctest.h>
#include "idoc/serde/paragraph_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::Run make_sample_run() {
    model::Run run;
    run.run_id = "run-1";
    run.text = "In the shadow of the ash mountains,";

    model::RunProperties props;
    props.bold = true;
    props.italic = false;
    props.size_pt = 12.5f;
    props.font = model::FontRef{"Garamond", "Times New Roman"};
    props.color = model::Color{0x1A, 0x1A, 0x1A, 255};
    props.underline = model::underline_value::kSingle;
    props.vertical_align = model::TextVerticalAlign::kBaseline;
    run.direct_props = props;

    run.hyperlink_id = "link-1";
    run.comment_anchor_ids = std::vector<std::string>{"comment-1", "comment-2"};

    return run;
}

model::Paragraph make_sample_paragraph() {
    model::Paragraph p;
    p.paragraph_id = "para-1";
    p.style_id = "Heading1";

    model::ParagraphProperties props;
    props.alignment = model::Alignment::kJustify;
    props.indent = {720, 0, 360, 0};
    props.spacing = {240, 120, 360, model::LineRule::kMultiple};
    props.keep_with_next = true;
    props.page_break_before = false;
    p.direct_props = props;

    p.list_ref = model::ListRef{"list-instance-1", 0};
    p.runs = {make_sample_run()};

    return p;
}

} // namespace

TEST_CASE("paragraph serde: full round-trip") {
    model::DocumentContent dc;
    dc.paragraphs = {make_sample_paragraph()};

    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);

    CHECK(dc2 == dc);
}

TEST_CASE("paragraph serde: minimal paragraph (no optional fields) round-trips") {
    model::Paragraph p;
    p.paragraph_id = "para-minimal";
    // style_id, direct_props, list_ref all unset; no runs

    model::DocumentContent dc;
    dc.paragraphs = {p};

    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);

    REQUIRE(dc2.paragraphs.size() == 1);
    CHECK_FALSE(dc2.paragraphs[0].style_id.has_value());
    CHECK_FALSE(dc2.paragraphs[0].direct_props.has_value());
    CHECK_FALSE(dc2.paragraphs[0].list_ref.has_value());
    CHECK(dc2.paragraphs[0].runs.empty());
    CHECK(dc2 == dc);
}

TEST_CASE("paragraph serde: multiple runs preserve order") {
    model::Run r1; r1.run_id = "r1"; r1.text = "Hello, ";
    model::Run r2; r2.run_id = "r2"; r2.text = "world";
    model::RunProperties bold_props; bold_props.bold = true;
    r2.direct_props = bold_props;
    model::Run r3; r3.run_id = "r3"; r3.text = "!";

    model::Paragraph p;
    p.paragraph_id = "para-multi-run";
    p.runs = {r1, r2, r3};

    model::DocumentContent dc;
    dc.paragraphs = {p};

    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);

    REQUIRE(dc2.paragraphs.size() == 1);
    REQUIRE(dc2.paragraphs[0].runs.size() == 3);
    CHECK(dc2.paragraphs[0].runs[0].text == "Hello, ");
    CHECK(dc2.paragraphs[0].runs[1].text == "world");
    CHECK(dc2.paragraphs[0].runs[1].direct_props->bold.value() == true);
    CHECK(dc2.paragraphs[0].runs[2].text == "!");
}

TEST_CASE("paragraph serde: RunProperties optional fields round-trip individually") {
    model::Run r;
    r.run_id = "r1";
    r.text = "text";
    model::RunProperties props;
    props.small_caps = true;
    // everything else deliberately left unset
    r.direct_props = props;

    model::Paragraph p;
    p.paragraph_id = "p1";
    p.runs = {r};

    model::DocumentContent dc;
    dc.paragraphs = {p};

    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);

    const auto& rp = dc2.paragraphs[0].runs[0].direct_props.value();
    CHECK(rp.small_caps.value() == true);
    CHECK_FALSE(rp.bold.has_value());
    CHECK_FALSE(rp.font.has_value());
    CHECK_FALSE(rp.color.has_value());
    CHECK_FALSE(rp.size_pt.has_value());
}

TEST_CASE("paragraph serde: float fields (size_pt) survive round-trip exactly") {
    model::Run r;
    r.run_id = "r1";
    r.text = "text";
    model::RunProperties props;
    props.size_pt = 11.5f;
    props.character_spacing_pt = 0.25f;
    r.direct_props = props;

    model::Paragraph p;
    p.paragraph_id = "p1";
    p.runs = {r};

    model::DocumentContent dc;
    dc.paragraphs = {p};

    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);

    CHECK(dc2.paragraphs[0].runs[0].direct_props->size_pt.value() == doctest::Approx(11.5f));
    CHECK(dc2.paragraphs[0].runs[0].direct_props->character_spacing_pt.value() == doctest::Approx(0.25f));
}

TEST_CASE("paragraph serde: an unrecognized underline value round-trips untouched") {
    // Underline is explicitly open-ended in the spec -- a future minor
    // version's value must survive even though this reader doesn't know
    // what it means semantically. See model/paragraph.hpp.
    model::Run r;
    r.run_id = "r1";
    r.text = "text";
    model::RunProperties props;
    props.underline = 200; // not one of the named underline_value constants
    r.direct_props = props;

    model::Paragraph p;
    p.paragraph_id = "p1";
    p.runs = {r};

    model::DocumentContent dc;
    dc.paragraphs = {p};

    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);

    CHECK(dc2.paragraphs[0].runs[0].direct_props->underline.value() == 200);
}

TEST_CASE("paragraph serde: empty document content round-trips") {
    model::DocumentContent dc;
    auto payload = serde::serialize_document_content(dc);
    auto dc2 = serde::deserialize_document_content(payload);
    CHECK(dc2.paragraphs.empty());
}

TEST_CASE("paragraph serde: unknown field within a Paragraph is skipped") {
    model::DocumentContent dc;
    dc.paragraphs = {make_sample_paragraph()};
    auto payload = serde::serialize_document_content(dc);

    auto outer_records = tlv::parse_records(payload);
    REQUIRE(outer_records.size() == 1);
    std::vector<uint8_t> patched_inner = outer_records[0].payload;
    tlv::write_record(patched_inner, 999, 1, {0x01});

    std::vector<uint8_t> patched_payload;
    tlv::write_record(patched_payload, outer_records[0].header.type_id,
                       outer_records[0].header.schema_version, patched_inner);

    auto dc2 = serde::deserialize_document_content(patched_payload);
    REQUIRE(dc2.paragraphs.size() == 1);
    CHECK(dc2.paragraphs[0].paragraph_id == "para-1");
}
