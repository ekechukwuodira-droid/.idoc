#include "idoc/serde/common_serde.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/container/tlv.hpp"

namespace idoc::serde::common {

namespace run_props_field {
// Fields nested within a RunProperties record's payload.
constexpr uint32_t kStyleId = 1;
constexpr uint32_t kFont = 2;
constexpr uint32_t kSizePt = 3;
constexpr uint32_t kBold = 4;
constexpr uint32_t kItalic = 5;
constexpr uint32_t kUnderline = 6;
constexpr uint32_t kStrikethrough = 7;
constexpr uint32_t kVerticalAlign = 8;
constexpr uint32_t kColor = 9;
constexpr uint32_t kHighlight = 10;
constexpr uint32_t kCharacterSpacingPt = 11;
constexpr uint32_t kSmallCaps = 12;
constexpr uint32_t kAllCaps = 13;
} // namespace run_props_field

namespace para_props_field {
// Fields nested within a ParagraphProperties record's payload.
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

std::vector<uint8_t> encode_color(const model::Color& c) {
    std::vector<uint8_t> out;
    byteorder::write_u8(out, c.r);
    byteorder::write_u8(out, c.g);
    byteorder::write_u8(out, c.b);
    byteorder::write_u8(out, c.a);
    return out;
}

model::Color decode_color(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::Color c;
    c.r = r.read_u8();
    c.g = r.read_u8();
    c.b = r.read_u8();
    c.a = r.read_u8();
    return c;
}

std::vector<uint8_t> encode_font_ref(const model::FontRef& f) {
    std::vector<uint8_t> out;
    byteorder::write_string(out, f.family);
    byteorder::write_u8(out, f.fallback.has_value() ? 1 : 0);
    if (f.fallback.has_value()) byteorder::write_string(out, *f.fallback);
    return out;
}

model::FontRef decode_font_ref(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::FontRef f;
    f.family = r.read_string();
    if (r.read_u8() != 0) f.fallback = r.read_string();
    return f;
}

std::vector<uint8_t> encode_indent(const model::Indent& i) {
    std::vector<uint8_t> out;
    byteorder::write_u32(out, static_cast<uint32_t>(i.left));
    byteorder::write_u32(out, static_cast<uint32_t>(i.right));
    byteorder::write_u32(out, static_cast<uint32_t>(i.first_line));
    byteorder::write_u32(out, static_cast<uint32_t>(i.hanging));
    return out;
}

model::Indent decode_indent(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::Indent i;
    i.left = static_cast<int32_t>(r.read_u32());
    i.right = static_cast<int32_t>(r.read_u32());
    i.first_line = static_cast<int32_t>(r.read_u32());
    i.hanging = static_cast<int32_t>(r.read_u32());
    return i;
}

std::vector<uint8_t> encode_size2d(const model::Size2D& s) {
    std::vector<uint8_t> out;
    byteorder::write_u32(out, s.width);
    byteorder::write_u32(out, s.height);
    return out;
}

model::Size2D decode_size2d(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::Size2D s;
    s.width = r.read_u32();
    s.height = r.read_u32();
    return s;
}

std::vector<uint8_t> encode_spacing(const model::Spacing& s) {
    std::vector<uint8_t> out;
    byteorder::write_u32(out, s.before);
    byteorder::write_u32(out, s.after);
    byteorder::write_u32(out, static_cast<uint32_t>(s.line));
    byteorder::write_u8(out, static_cast<uint8_t>(s.line_rule));
    return out;
}

model::Spacing decode_spacing(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::Spacing s;
    s.before = r.read_u32();
    s.after = r.read_u32();
    s.line = static_cast<int32_t>(r.read_u32());
    s.line_rule = static_cast<model::LineRule>(r.read_u8());
    return s;
}

std::vector<uint8_t> encode_run_properties(const model::RunProperties& rp, uint16_t schema_version) {
    std::vector<uint8_t> out;

    if (rp.style_id.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *rp.style_id);
        tlv::write_record(out, run_props_field::kStyleId, schema_version, p);
    }
    if (rp.font.has_value()) {
        tlv::write_record(out, run_props_field::kFont, schema_version, encode_font_ref(*rp.font));
    }
    if (rp.size_pt.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_f32(p, *rp.size_pt);
        tlv::write_record(out, run_props_field::kSizePt, schema_version, p);
    }
    if (rp.bold.has_value()) {
        tlv::write_record(out, run_props_field::kBold, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*rp.bold ? 1 : 0)});
    }
    if (rp.italic.has_value()) {
        tlv::write_record(out, run_props_field::kItalic, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*rp.italic ? 1 : 0)});
    }
    if (rp.underline.has_value()) {
        tlv::write_record(out, run_props_field::kUnderline, schema_version,
                           std::vector<uint8_t>{*rp.underline});
    }
    if (rp.strikethrough.has_value()) {
        tlv::write_record(out, run_props_field::kStrikethrough, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*rp.strikethrough ? 1 : 0)});
    }
    if (rp.vertical_align.has_value()) {
        tlv::write_record(out, run_props_field::kVerticalAlign, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*rp.vertical_align)});
    }
    if (rp.color.has_value()) {
        tlv::write_record(out, run_props_field::kColor, schema_version, encode_color(*rp.color));
    }
    if (rp.highlight.has_value()) {
        tlv::write_record(out, run_props_field::kHighlight, schema_version, encode_color(*rp.highlight));
    }
    if (rp.character_spacing_pt.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_f32(p, *rp.character_spacing_pt);
        tlv::write_record(out, run_props_field::kCharacterSpacingPt, schema_version, p);
    }
    if (rp.small_caps.has_value()) {
        tlv::write_record(out, run_props_field::kSmallCaps, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*rp.small_caps ? 1 : 0)});
    }
    if (rp.all_caps.has_value()) {
        tlv::write_record(out, run_props_field::kAllCaps, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*rp.all_caps ? 1 : 0)});
    }

    return out;
}

model::RunProperties decode_run_properties(const std::vector<uint8_t>& payload) {
    model::RunProperties rp;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case run_props_field::kStyleId:
                rp.style_id = r.read_string();
                break;
            case run_props_field::kFont:
                rp.font = decode_font_ref(rec.payload);
                break;
            case run_props_field::kSizePt:
                rp.size_pt = r.read_f32();
                break;
            case run_props_field::kBold:
                rp.bold = r.read_u8() != 0;
                break;
            case run_props_field::kItalic:
                rp.italic = r.read_u8() != 0;
                break;
            case run_props_field::kUnderline:
                rp.underline = r.read_u8();
                break;
            case run_props_field::kStrikethrough:
                rp.strikethrough = r.read_u8() != 0;
                break;
            case run_props_field::kVerticalAlign:
                rp.vertical_align = static_cast<model::TextVerticalAlign>(r.read_u8());
                break;
            case run_props_field::kColor:
                rp.color = decode_color(rec.payload);
                break;
            case run_props_field::kHighlight:
                rp.highlight = decode_color(rec.payload);
                break;
            case run_props_field::kCharacterSpacingPt:
                rp.character_spacing_pt = r.read_f32();
                break;
            case run_props_field::kSmallCaps:
                rp.small_caps = r.read_u8() != 0;
                break;
            case run_props_field::kAllCaps:
                rp.all_caps = r.read_u8() != 0;
                break;
            default:
                break; // unknown field: skip
        }
    }

    return rp;
}

std::vector<uint8_t> encode_restart_rule(const model::RestartRule& rr) {
    std::vector<uint8_t> out;
    byteorder::write_u8(out, static_cast<uint8_t>(rr.kind));
    byteorder::write_u8(out, rr.level_index.has_value() ? 1 : 0);
    if (rr.level_index.has_value()) byteorder::write_u32(out, static_cast<uint32_t>(*rr.level_index));
    return out;
}

model::RestartRule decode_restart_rule(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::RestartRule rr;
    rr.kind = static_cast<model::RestartRule::Kind>(r.read_u8());
    if (r.read_u8() != 0) rr.level_index = static_cast<int32_t>(r.read_u32());
    return rr;
}

namespace {
constexpr uint32_t kContentRefRecordType = 1; // fixed record type_id within a content-ref list payload
} // namespace

std::vector<uint8_t> encode_content_ref(const model::ContentRef& ref) {
    std::vector<uint8_t> out;
    byteorder::write_u8(out, static_cast<uint8_t>(ref.type));
    byteorder::write_string(out, ref.content_id);
    return out;
}

model::ContentRef decode_content_ref(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::ContentRef ref;
    ref.type = static_cast<model::ContentType>(r.read_u8());
    ref.content_id = r.read_string();
    return ref;
}

std::vector<uint8_t> encode_content_refs(const std::vector<model::ContentRef>& refs) {
    std::vector<uint8_t> out;
    for (const auto& ref : refs) {
        tlv::write_record(out, kContentRefRecordType, 1, encode_content_ref(ref));
    }
    return out;
}

std::vector<model::ContentRef> decode_content_refs(const std::vector<uint8_t>& payload) {
    std::vector<model::ContentRef> refs;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != kContentRefRecordType) continue; // unknown: skip
        refs.push_back(decode_content_ref(rec.payload));
    }
    return refs;
}

std::vector<uint8_t> encode_paragraph_properties(const model::ParagraphProperties& pp, uint16_t schema_version) {
    std::vector<uint8_t> out;

    if (pp.alignment.has_value()) {
        tlv::write_record(out, para_props_field::kAlignment, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*pp.alignment)});
    }
    if (pp.indent.has_value()) {
        tlv::write_record(out, para_props_field::kIndent, schema_version, encode_indent(*pp.indent));
    }
    if (pp.spacing.has_value()) {
        tlv::write_record(out, para_props_field::kSpacing, schema_version, encode_spacing(*pp.spacing));
    }
    if (pp.keep_with_next.has_value()) {
        tlv::write_record(out, para_props_field::kKeepWithNext, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*pp.keep_with_next ? 1 : 0)});
    }
    if (pp.keep_lines_together.has_value()) {
        tlv::write_record(out, para_props_field::kKeepLinesTogether, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*pp.keep_lines_together ? 1 : 0)});
    }
    if (pp.page_break_before.has_value()) {
        tlv::write_record(out, para_props_field::kPageBreakBefore, schema_version,
                           std::vector<uint8_t>{static_cast<uint8_t>(*pp.page_break_before ? 1 : 0)});
    }
    if (pp.borders_raw.has_value()) {
        tlv::write_record(out, para_props_field::kBordersRaw, schema_version, *pp.borders_raw);
    }
    if (pp.shading_raw.has_value()) {
        tlv::write_record(out, para_props_field::kShadingRaw, schema_version, *pp.shading_raw);
    }
    if (pp.tab_stops_raw.has_value()) {
        tlv::write_record(out, para_props_field::kTabStopsRaw, schema_version, *pp.tab_stops_raw);
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
                pp.indent = decode_indent(rec.payload);
                break;
            case para_props_field::kSpacing:
                pp.spacing = decode_spacing(rec.payload);
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

} // namespace idoc::serde::common
