#include "idoc/serde/fields_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

namespace idoc::serde {

namespace record_type {
constexpr uint32_t kField = 1; // only repeated record type at the block's top level
} // namespace record_type

namespace anchor_field {
constexpr uint32_t kX = 1;
constexpr uint32_t kY = 2;
} // namespace anchor_field

namespace format_field {
constexpr uint32_t kKind = 1;
constexpr uint32_t kWidth = 2;
} // namespace format_field

namespace payload_field {
// Fields nested within a PageNumberFieldPayload record's payload.
constexpr uint32_t kPosition = 1;
constexpr uint32_t kAnchor = 2;
constexpr uint32_t kFormat = 3;
constexpr uint32_t kIncludeTotalPages = 4;
constexpr uint32_t kIncludeChapterNumber = 5;
constexpr uint32_t kIncludeSectionNumber = 6;
constexpr uint32_t kRestartRule = 7;
constexpr uint32_t kDifferentFirstPage = 8;
constexpr uint32_t kDifferentOddEven = 9;
constexpr uint32_t kPrefix = 10;
constexpr uint32_t kSuffix = 11;
constexpr uint32_t kPattern = 12;
constexpr uint32_t kNumberStyle = 13;
} // namespace payload_field

namespace field_field {
constexpr uint32_t kFieldId = 1;
constexpr uint32_t kType = 2;
constexpr uint32_t kPageNumberPayload = 3;
constexpr uint32_t kOtherPayloadRaw = 4;
constexpr uint32_t kCachedResult = 5;
constexpr uint32_t kCachedAt = 6;
constexpr uint32_t kLocked = 7;
} // namespace field_field

namespace {

std::vector<uint8_t> encode_anchor(const model::CustomAnchor& a) {
    std::vector<uint8_t> out;
    tlv::write_record(out, anchor_field::kX, kFieldsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, static_cast<uint32_t>(a.x)); return p; }());
    tlv::write_record(out, anchor_field::kY, kFieldsSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, static_cast<uint32_t>(a.y)); return p; }());
    return out;
}

model::CustomAnchor decode_anchor(const std::vector<uint8_t>& payload) {
    model::CustomAnchor a;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        if (rec.header.type_id == anchor_field::kX) a.x = static_cast<int32_t>(r.read_u32());
        else if (rec.header.type_id == anchor_field::kY) a.y = static_cast<int32_t>(r.read_u32());
        // unknown field: skip
    }
    return a;
}

std::vector<uint8_t> encode_format(const model::PageNumberFormat& f) {
    std::vector<uint8_t> out;
    tlv::write_record(out, format_field::kKind, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(f.kind)});
    if (f.width.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_u32(p, *f.width);
        tlv::write_record(out, format_field::kWidth, kFieldsSchemaVersion, p);
    }
    return out;
}

model::PageNumberFormat decode_format(const std::vector<uint8_t>& payload) {
    model::PageNumberFormat f;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        if (rec.header.type_id == format_field::kKind) {
            f.kind = static_cast<model::PageNumberFormat::Kind>(r.read_u8());
        } else if (rec.header.type_id == format_field::kWidth) {
            f.width = r.read_u32();
        }
        // unknown field: skip
    }
    return f;
}

std::vector<uint8_t> encode_page_number_payload(const model::PageNumberFieldPayload& p) {
    std::vector<uint8_t> out;

    tlv::write_record(out, payload_field::kPosition, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.position)});
    if (p.anchor.has_value()) {
        tlv::write_record(out, payload_field::kAnchor, kFieldsSchemaVersion, encode_anchor(*p.anchor));
    }
    tlv::write_record(out, payload_field::kFormat, kFieldsSchemaVersion, encode_format(p.format));
    tlv::write_record(out, payload_field::kIncludeTotalPages, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.include_total_pages ? 1 : 0)});
    tlv::write_record(out, payload_field::kIncludeChapterNumber, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.include_chapter_number ? 1 : 0)});
    tlv::write_record(out, payload_field::kIncludeSectionNumber, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.include_section_number ? 1 : 0)});
    tlv::write_record(out, payload_field::kRestartRule, kFieldsSchemaVersion,
                       common::encode_restart_rule(p.restart_rule));
    tlv::write_record(out, payload_field::kDifferentFirstPage, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.different_first_page ? 1 : 0)});
    tlv::write_record(out, payload_field::kDifferentOddEven, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(p.different_odd_even ? 1 : 0)});

    if (p.prefix.has_value()) {
        std::vector<uint8_t> pp;
        byteorder::write_string(pp, *p.prefix);
        tlv::write_record(out, payload_field::kPrefix, kFieldsSchemaVersion, pp);
    }
    if (p.suffix.has_value()) {
        std::vector<uint8_t> pp;
        byteorder::write_string(pp, *p.suffix);
        tlv::write_record(out, payload_field::kSuffix, kFieldsSchemaVersion, pp);
    }
    {
        std::vector<uint8_t> pp;
        byteorder::write_string(pp, p.pattern);
        tlv::write_record(out, payload_field::kPattern, kFieldsSchemaVersion, pp);
    }
    if (p.number_style.has_value()) {
        tlv::write_record(out, payload_field::kNumberStyle, kFieldsSchemaVersion,
                           common::encode_run_properties(*p.number_style, kFieldsSchemaVersion));
    }

    return out;
}

model::PageNumberFieldPayload decode_page_number_payload(const std::vector<uint8_t>& payload) {
    model::PageNumberFieldPayload p;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case payload_field::kPosition:
                p.position = static_cast<model::PageNumberPosition>(r.read_u8());
                break;
            case payload_field::kAnchor:
                p.anchor = decode_anchor(rec.payload);
                break;
            case payload_field::kFormat:
                p.format = decode_format(rec.payload);
                break;
            case payload_field::kIncludeTotalPages:
                p.include_total_pages = r.read_u8() != 0;
                break;
            case payload_field::kIncludeChapterNumber:
                p.include_chapter_number = r.read_u8() != 0;
                break;
            case payload_field::kIncludeSectionNumber:
                p.include_section_number = r.read_u8() != 0;
                break;
            case payload_field::kRestartRule:
                p.restart_rule = common::decode_restart_rule(rec.payload);
                break;
            case payload_field::kDifferentFirstPage:
                p.different_first_page = r.read_u8() != 0;
                break;
            case payload_field::kDifferentOddEven:
                p.different_odd_even = r.read_u8() != 0;
                break;
            case payload_field::kPrefix:
                p.prefix = r.read_string();
                break;
            case payload_field::kSuffix:
                p.suffix = r.read_string();
                break;
            case payload_field::kPattern:
                p.pattern = r.read_string();
                break;
            case payload_field::kNumberStyle:
                p.number_style = common::decode_run_properties(rec.payload);
                break;
            default:
                break; // unknown field: skip
        }
    }
    return p;
}

std::vector<uint8_t> encode_field(const model::Field& field) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, field.field_id);
        tlv::write_record(out, field_field::kFieldId, kFieldsSchemaVersion, p);
    }
    tlv::write_record(out, field_field::kType, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(field.type)});

    if (field.page_number_payload.has_value()) {
        tlv::write_record(out, field_field::kPageNumberPayload, kFieldsSchemaVersion,
                           encode_page_number_payload(*field.page_number_payload));
    }
    if (field.other_payload_raw.has_value()) {
        tlv::write_record(out, field_field::kOtherPayloadRaw, kFieldsSchemaVersion, *field.other_payload_raw);
    }

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, field.cached_result);
        tlv::write_record(out, field_field::kCachedResult, kFieldsSchemaVersion, p);
    }
    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, field.cached_at);
        tlv::write_record(out, field_field::kCachedAt, kFieldsSchemaVersion, p);
    }
    tlv::write_record(out, field_field::kLocked, kFieldsSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(field.locked ? 1 : 0)});

    return out;
}

model::Field decode_field(const std::vector<uint8_t>& payload) {
    model::Field field;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case field_field::kFieldId:
                field.field_id = r.read_string();
                break;
            case field_field::kType:
                field.type = static_cast<model::FieldType>(r.read_u8());
                break;
            case field_field::kPageNumberPayload:
                field.page_number_payload = decode_page_number_payload(rec.payload);
                break;
            case field_field::kOtherPayloadRaw:
                field.other_payload_raw = rec.payload;
                break;
            case field_field::kCachedResult:
                field.cached_result = r.read_string();
                break;
            case field_field::kCachedAt:
                field.cached_at = r.read_string();
                break;
            case field_field::kLocked:
                field.locked = r.read_u8() != 0;
                break;
            default:
                break; // unknown field: skip
        }
    }
    return field;
}

} // namespace

std::vector<uint8_t> serialize_fields(const model::Fields& fields) {
    std::vector<uint8_t> out;
    for (const auto& f : fields.fields) {
        tlv::write_record(out, record_type::kField, kFieldsSchemaVersion, encode_field(f));
    }
    return out;
}

model::Fields deserialize_fields(const std::vector<uint8_t>& payload) {
    model::Fields fields;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kField) continue; // unknown top-level record: skip
        fields.fields.push_back(decode_field(rec.payload));
    }
    return fields;
}

} // namespace idoc::serde
