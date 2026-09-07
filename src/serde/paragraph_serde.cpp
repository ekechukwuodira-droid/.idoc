#include "idoc/serde/paragraph_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

namespace idoc::serde {

namespace record_type {
constexpr uint32_t kParagraph = 1; // only repeated record type at the block's top level
} // namespace record_type

namespace para_props_field {
constexpr uint32_t kAlignment = 1;
constexpr uint32_t kIndent = 2;
constexpr uint32_t kSpacing = 3;
constexpr uint32_t kKeepWithNext = 4;
constexpr uint32_t kKeepLinesTogether = 5;
constexpr uint32_t kPageBreakBefore = 6;
constexpr uint32_t kBordersRaw = 7;   // reserved, see model/paragraph.hpp
constexpr uint32_t kShadingRaw = 8;   // reserved
constexpr uint32_t kTabStopsRaw = 9;  // reserved
} // namespace para_props_field

namespace run_field {
constexpr uint32_t kRunId = 1;
constexpr uint32_t kText = 2;
constexpr uint32_t kDirectProps = 3;
constexpr uint32_t kFieldRef = 4; // was raw bytes; now a real FieldRef (see model/paragraph.hpp)
constexpr uint32_t kHyperlinkId = 5;
constexpr uint32_t kCommentAnchorIds = 6;
} // namespace run_field

namespace paragraph_field {
constexpr uint32_t kParagraphId = 1;
constexpr uint32_t kStyleId = 2;
constexpr uint32_t kDirectProps = 3;
constexpr uint32_t kListRef = 4;
constexpr uint32_t kRun = 5; // repeated
} // namespace paragraph_field

namespace {

std::vector<uint8_t> encode_paragraph_properties(const model::ParagraphProperties& pp) {
    std::vector<uint8_t> out;

    tlv::write_record(out, para_props_field::kAlignment, kParagraphsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(pp.alignment)});
    tlv::write_record(out, para_props_field::kIndent, kParagraphsSchemaVersion,
                       common::encode_indent(pp.indent));
    tlv::write_record(out, para_props_field::kSpacing, kParagraphsSchemaVersion,
                       common::encode_spacing(pp.spacing));
    tlv::write_record(out, para_props_field::kKeepWithNext, kParagraphsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(pp.keep_with_next ? 1 : 0)});
    tlv::write_record(out, para_props_field::kKeepLinesTogether, kParagraphsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(pp.keep_lines_together ? 1 : 0)});
    tlv::write_record(out, para_props_field::kPageBreakBefore, kParagraphsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(pp.page_break_before ? 1 : 0)});

    if (pp.borders_raw.has_value()) {
        tlv::write_record(out, para_props_field::kBordersRaw, kParagraphsSchemaVersion, *pp.borders_raw);
    }
    if (pp.shading_raw.has_value()) {
        tlv::write_record(out, para_props_field::kShadingRaw, kParagraphsSchemaVersion, *pp.shading_raw);
    }
    if (pp.tab_stops_raw.has_value()) {
        tlv::write_record(out, para_props_field::kTabStopsRaw, kParagraphsSchemaVersion, *pp.tab_stops_raw);
    }

    return out;
}

model::ParagraphProperties decode_paragraph_properties(const std::vector<uint8_t>& payload) {
    model::ParagraphProperties pp;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case para_props_field::kAlignment:
                pp.alignment = static_cast<model::Alignment>(r.read_u8());
                break;
            case para_props_field::kIndent:
                pp.indent = common::decode_indent(rec.payload);
                break;
            case para_props_field::kSpacing:
                pp.spacing = common::decode_spacing(rec.payload);
                break;
            case para_props_field::kKeepWithNext:
                pp.keep_with_next = r.read_u8() != 0;
                break;
            case para_props_field::kKeepLinesTogether:
                pp.keep_lines_together = r.read_u8() != 0;
                break;
            case para_props_field::kPageBreakBefore:
                pp.page_break_before = r.read_u8() != 0;
                break;
            case para_props_field::kBordersRaw:
                pp.borders_raw = rec.payload;
                break;
            case para_props_field::kShadingRaw:
                pp.shading_raw = rec.payload;
                break;
            case para_props_field::kTabStopsRaw:
                pp.tab_stops_raw = rec.payload;
                break;
            default:
                break; // unknown field: skip
        }
    }
    return pp;
}

std::vector<uint8_t> encode_run(const model::Run& run) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, run.run_id);
        tlv::write_record(out, run_field::kRunId, kParagraphsSchemaVersion, p);
    }
    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, run.text);
        tlv::write_record(out, run_field::kText, kParagraphsSchemaVersion, p);
    }
    if (run.direct_props.has_value()) {
        tlv::write_record(out, run_field::kDirectProps, kParagraphsSchemaVersion,
                           common::encode_run_properties(*run.direct_props, kParagraphsSchemaVersion));
    }
    if (run.field_ref.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, run.field_ref->field_id);
        tlv::write_record(out, run_field::kFieldRef, kParagraphsSchemaVersion, p);
    }
    if (run.hyperlink_id.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *run.hyperlink_id);
        tlv::write_record(out, run_field::kHyperlinkId, kParagraphsSchemaVersion, p);
    }
    if (run.comment_anchor_ids.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_u32(p, static_cast<uint32_t>(run.comment_anchor_ids->size()));
        for (const auto& id : *run.comment_anchor_ids) byteorder::write_string(p, id);
        tlv::write_record(out, run_field::kCommentAnchorIds, kParagraphsSchemaVersion, p);
    }

    return out;
}

model::Run decode_run(const std::vector<uint8_t>& payload) {
    model::Run run;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case run_field::kRunId:
                run.run_id = r.read_string();
                break;
            case run_field::kText:
                run.text = r.read_string();
                break;
            case run_field::kDirectProps:
                run.direct_props = common::decode_run_properties(rec.payload);
                break;
            case run_field::kFieldRef: {
                model::FieldRef fr;
                fr.field_id = r.read_string();
                run.field_ref = fr;
                break;
            }
            case run_field::kHyperlinkId:
                run.hyperlink_id = r.read_string();
                break;
            case run_field::kCommentAnchorIds: {
                uint32_t count = r.read_u32();
                std::vector<std::string> ids;
                ids.reserve(count);
                for (uint32_t i = 0; i < count; ++i) ids.push_back(r.read_string());
                run.comment_anchor_ids = std::move(ids);
                break;
            }
            default:
                break; // unknown field: skip
        }
    }
    return run;
}

std::vector<uint8_t> encode_paragraph(const model::Paragraph& para) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, para.paragraph_id);
        tlv::write_record(out, paragraph_field::kParagraphId, kParagraphsSchemaVersion, p);
    }
    if (para.style_id.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *para.style_id);
        tlv::write_record(out, paragraph_field::kStyleId, kParagraphsSchemaVersion, p);
    }
    if (para.direct_props.has_value()) {
        tlv::write_record(out, paragraph_field::kDirectProps, kParagraphsSchemaVersion,
                           encode_paragraph_properties(*para.direct_props));
    }
    if (para.list_ref.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, para.list_ref->instance_id);
        byteorder::write_u32(p, static_cast<uint32_t>(para.list_ref->level));
        tlv::write_record(out, paragraph_field::kListRef, kParagraphsSchemaVersion, p);
    }
    for (const auto& run : para.runs) {
        tlv::write_record(out, paragraph_field::kRun, kParagraphsSchemaVersion, encode_run(run));
    }

    return out;
}

model::Paragraph decode_paragraph(const std::vector<uint8_t>& payload) {
    model::Paragraph para;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case paragraph_field::kParagraphId:
                para.paragraph_id = r.read_string();
                break;
            case paragraph_field::kStyleId:
                para.style_id = r.read_string();
                break;
            case paragraph_field::kDirectProps:
                para.direct_props = decode_paragraph_properties(rec.payload);
                break;
            case paragraph_field::kListRef: {
                model::ListRef lr;
                lr.instance_id = r.read_string();
                lr.level = static_cast<int32_t>(r.read_u32());
                para.list_ref = lr;
                break;
            }
            case paragraph_field::kRun:
                para.runs.push_back(decode_run(rec.payload));
                break;
            default:
                break; // unknown field: skip
        }
    }
    return para;
}

} // namespace

std::vector<uint8_t> serialize_document_content(const model::DocumentContent& dc) {
    std::vector<uint8_t> out;
    for (const auto& para : dc.paragraphs) {
        tlv::write_record(out, record_type::kParagraph, kParagraphsSchemaVersion, encode_paragraph(para));
    }
    return out;
}

model::DocumentContent deserialize_document_content(const std::vector<uint8_t>& payload) {
    model::DocumentContent dc;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kParagraph) continue; // unknown top-level record: skip
        dc.paragraphs.push_back(decode_paragraph(rec.payload));
    }
    return dc;
}

} // namespace idoc::serde
