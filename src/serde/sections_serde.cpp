#include "idoc/serde/sections_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

#include <stdexcept>

namespace idoc::serde {

namespace record_type {
constexpr uint32_t kSection = 1; // only repeated record type at the block's top level
} // namespace record_type

namespace section_field {
constexpr uint32_t kSectionId = 1;
constexpr uint32_t kPageSetup = 2;
constexpr uint32_t kHeaders = 3;
constexpr uint32_t kFooters = 4;
constexpr uint32_t kPageNumberRestart = 5;
constexpr uint32_t kContentRaw = 6; // reserved, see DEFERRED note in sections.hpp
} // namespace section_field

namespace page_setup_field {
constexpr uint32_t kPageSize = 1;
constexpr uint32_t kMargins = 2;
constexpr uint32_t kOrientation = 3;
constexpr uint32_t kColumns = 4;
constexpr uint32_t kDifferentFirstPage = 5;
constexpr uint32_t kDifferentOddEven = 6;
constexpr uint32_t kPaperSource = 7;
} // namespace page_setup_field

namespace header_footer_set_field {
constexpr uint32_t kDefault = 1;
constexpr uint32_t kFirst = 2;
constexpr uint32_t kEven = 3;
} // namespace header_footer_set_field

namespace header_footer_content_field {
constexpr uint32_t kContentRaw = 1; // reserved, see DEFERRED note in sections.hpp
} // namespace header_footer_content_field

namespace numbering_restart_field {
constexpr uint32_t kRestart = 1;
constexpr uint32_t kStartAt = 2;
} // namespace numbering_restart_field

namespace {

// --- Margins / ColumnLayout: simple fixed-shape payloads, no nested TLV needed ---
// (Size2D moved to common_serde -- also needed by §11's ResourceEntry.natural_size)

std::vector<uint8_t> encode_margins(const model::Margins& m) {
    std::vector<uint8_t> out;
    byteorder::write_u32(out, static_cast<uint32_t>(m.top));
    byteorder::write_u32(out, static_cast<uint32_t>(m.bottom));
    byteorder::write_u32(out, static_cast<uint32_t>(m.left));
    byteorder::write_u32(out, static_cast<uint32_t>(m.right));
    byteorder::write_u32(out, m.gutter);
    return out;
}
model::Margins decode_margins(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::Margins m;
    m.top = static_cast<int32_t>(r.read_u32());
    m.bottom = static_cast<int32_t>(r.read_u32());
    m.left = static_cast<int32_t>(r.read_u32());
    m.right = static_cast<int32_t>(r.read_u32());
    m.gutter = r.read_u32();
    return m;
}

std::vector<uint8_t> encode_columns(const model::ColumnLayout& c) {
    std::vector<uint8_t> out;
    byteorder::write_u32(out, c.count);
    byteorder::write_u32(out, c.spacing);
    byteorder::write_u8(out, c.equal_width ? 1 : 0);
    byteorder::write_u8(out, c.rule_line ? 1 : 0);
    return out;
}
model::ColumnLayout decode_columns(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::ColumnLayout c;
    c.count = r.read_u32();
    c.spacing = r.read_u32();
    c.equal_width = r.read_u8() != 0;
    c.rule_line = r.read_u8() != 0;
    return c;
}

// --- PageSetup: nested TLV field list ---

std::vector<uint8_t> encode_page_setup(const model::PageSetup& p) {
    std::vector<uint8_t> out;
    tlv::write_record(out, page_setup_field::kPageSize, kSectionsSchemaVersion, common::encode_size2d(p.page_size));
    tlv::write_record(out, page_setup_field::kMargins, kSectionsSchemaVersion, encode_margins(p.margins));
    tlv::write_record(out, page_setup_field::kOrientation, kSectionsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.orientation)});
    tlv::write_record(out, page_setup_field::kColumns, kSectionsSchemaVersion, encode_columns(p.columns));
    tlv::write_record(out, page_setup_field::kDifferentFirstPage, kSectionsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.different_first_page ? 1 : 0)});
    tlv::write_record(out, page_setup_field::kDifferentOddEven, kSectionsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.different_odd_even ? 1 : 0)});
    if (p.paper_source.has_value()) {
        std::vector<uint8_t> ps;
        byteorder::write_string(ps, *p.paper_source);
        tlv::write_record(out, page_setup_field::kPaperSource, kSectionsSchemaVersion, ps);
    }
    return out;
}

model::PageSetup decode_page_setup(const std::vector<uint8_t>& payload) {
    model::PageSetup p;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case page_setup_field::kPageSize:
                p.page_size = common::decode_size2d(rec.payload);
                break;
            case page_setup_field::kMargins:
                p.margins = decode_margins(rec.payload);
                break;
            case page_setup_field::kOrientation:
                p.orientation = static_cast<model::Orientation>(r.read_u8());
                break;
            case page_setup_field::kColumns:
                p.columns = decode_columns(rec.payload);
                break;
            case page_setup_field::kDifferentFirstPage:
                p.different_first_page = r.read_u8() != 0;
                break;
            case page_setup_field::kDifferentOddEven:
                p.different_odd_even = r.read_u8() != 0;
                break;
            case page_setup_field::kPaperSource:
                p.paper_source = r.read_string();
                break;
            default:
                break; // unknown field: skip
        }
    }
    return p;
}

// --- HeaderFooterContent / HeaderFooterSet ---

std::vector<uint8_t> encode_header_footer_content(const model::HeaderFooterContent& c) {
    std::vector<uint8_t> out;
    if (c.content_raw.has_value()) {
        tlv::write_record(out, header_footer_content_field::kContentRaw, kSectionsSchemaVersion, *c.content_raw);
    }
    return out;
}

model::HeaderFooterContent decode_header_footer_content(const std::vector<uint8_t>& payload) {
    model::HeaderFooterContent c;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id == header_footer_content_field::kContentRaw) {
            c.content_raw = rec.payload;
        }
        // unknown fields: skip
    }
    return c;
}

std::vector<uint8_t> encode_header_footer_set(const model::HeaderFooterSet& hfs) {
    std::vector<uint8_t> out;
    if (hfs.default_.has_value()) {
        tlv::write_record(out, header_footer_set_field::kDefault, kSectionsSchemaVersion,
                           encode_header_footer_content(*hfs.default_));
    }
    if (hfs.first.has_value()) {
        tlv::write_record(out, header_footer_set_field::kFirst, kSectionsSchemaVersion,
                           encode_header_footer_content(*hfs.first));
    }
    if (hfs.even.has_value()) {
        tlv::write_record(out, header_footer_set_field::kEven, kSectionsSchemaVersion,
                           encode_header_footer_content(*hfs.even));
    }
    return out;
}

model::HeaderFooterSet decode_header_footer_set(const std::vector<uint8_t>& payload) {
    model::HeaderFooterSet hfs;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        switch (rec.header.type_id) {
            case header_footer_set_field::kDefault:
                hfs.default_ = decode_header_footer_content(rec.payload);
                break;
            case header_footer_set_field::kFirst:
                hfs.first = decode_header_footer_content(rec.payload);
                break;
            case header_footer_set_field::kEven:
                hfs.even = decode_header_footer_content(rec.payload);
                break;
            default:
                break; // unknown field: skip
        }
    }
    return hfs;
}

// --- NumberingRestart ---

std::vector<uint8_t> encode_numbering_restart(const model::NumberingRestart& nr) {
    std::vector<uint8_t> out;
    tlv::write_record(out, numbering_restart_field::kRestart, kSectionsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(nr.restart ? 1 : 0)});
    if (nr.start_at.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_u32(p, *nr.start_at);
        tlv::write_record(out, numbering_restart_field::kStartAt, kSectionsSchemaVersion, p);
    }
    return out;
}

model::NumberingRestart decode_numbering_restart(const std::vector<uint8_t>& payload) {
    model::NumberingRestart nr;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case numbering_restart_field::kRestart:
                nr.restart = r.read_u8() != 0;
                break;
            case numbering_restart_field::kStartAt:
                nr.start_at = r.read_u32();
                break;
            default:
                break; // unknown field: skip
        }
    }
    return nr;
}

// --- Section ---

std::vector<uint8_t> encode_section(const model::Section& s) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, s.section_id);
        tlv::write_record(out, section_field::kSectionId, kSectionsSchemaVersion, p);
    }
    tlv::write_record(out, section_field::kPageSetup, kSectionsSchemaVersion, encode_page_setup(s.page_setup));
    tlv::write_record(out, section_field::kHeaders, kSectionsSchemaVersion, encode_header_footer_set(s.headers));
    tlv::write_record(out, section_field::kFooters, kSectionsSchemaVersion, encode_header_footer_set(s.footers));

    if (s.page_number_restart.has_value()) {
        tlv::write_record(out, section_field::kPageNumberRestart, kSectionsSchemaVersion,
                           encode_numbering_restart(*s.page_number_restart));
    }
    if (s.content_raw.has_value()) {
        tlv::write_record(out, section_field::kContentRaw, kSectionsSchemaVersion, *s.content_raw);
    }

    return out;
}

model::Section decode_section(const std::vector<uint8_t>& payload) {
    model::Section s;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        switch (rec.header.type_id) {
            case section_field::kSectionId: {
                byteorder::Reader r(rec.payload);
                s.section_id = r.read_string();
                break;
            }
            case section_field::kPageSetup:
                s.page_setup = decode_page_setup(rec.payload);
                break;
            case section_field::kHeaders:
                s.headers = decode_header_footer_set(rec.payload);
                break;
            case section_field::kFooters:
                s.footers = decode_header_footer_set(rec.payload);
                break;
            case section_field::kPageNumberRestart:
                s.page_number_restart = decode_numbering_restart(rec.payload);
                break;
            case section_field::kContentRaw:
                s.content_raw = rec.payload;
                break;
            default:
                break; // unknown field: skip
        }
    }

    return s;
}

} // namespace

std::vector<uint8_t> serialize_sections(const model::Sections& sections) {
    std::vector<uint8_t> out;
    for (const auto& s : sections.sections) {
        tlv::write_record(out, record_type::kSection, kSectionsSchemaVersion, encode_section(s));
    }
    return out;
}

model::Sections deserialize_sections(const std::vector<uint8_t>& payload) {
    model::Sections result;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kSection) continue; // unknown top-level record: skip
        result.sections.push_back(decode_section(rec.payload));
    }
    return result;
}

} // namespace idoc::serde
