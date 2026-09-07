#include "idoc/serde/styles_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"

#include <stdexcept>

namespace idoc::serde {

namespace record_type {
// Only one repeated record type lives at the top level of the Styles block.
constexpr uint32_t kStyleDefinition = 1;
} // namespace record_type

namespace field {
// Fields nested within a single StyleDefinition record's payload.
constexpr uint32_t kStyleId = 1;
constexpr uint32_t kDisplayName = 2;
constexpr uint32_t kType = 3;
constexpr uint32_t kBasedOn = 4;
constexpr uint32_t kNextStyle = 5;
constexpr uint32_t kIsDefault = 6;
constexpr uint32_t kQuickStyle = 7;
constexpr uint32_t kParagraphPropsRaw = 8; // reserved, always absent until §6 exists
constexpr uint32_t kRunPropsRaw = 9;       // reserved, always absent until §6 exists
} // namespace field

namespace {

std::vector<uint8_t> encode_style_definition(const model::StyleDefinition& s) {
    std::vector<uint8_t> out;

    tlv::write_record(out, field::kStyleId, kStylesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, s.style_id); return p; }());
    tlv::write_record(out, field::kDisplayName, kStylesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, s.display_name); return p; }());
    tlv::write_record(out, field::kType, kStylesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u8(p, static_cast<uint8_t>(s.type)); return p; }());

    if (s.based_on.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *s.based_on);
        tlv::write_record(out, field::kBasedOn, kStylesSchemaVersion, p);
    }
    if (s.next_style.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *s.next_style);
        tlv::write_record(out, field::kNextStyle, kStylesSchemaVersion, p);
    }

    tlv::write_record(out, field::kIsDefault, kStylesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u8(p, s.is_default ? 1 : 0); return p; }());
    tlv::write_record(out, field::kQuickStyle, kStylesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u8(p, s.quick_style ? 1 : 0); return p; }());

    if (s.paragraph_props_raw.has_value()) {
        tlv::write_record(out, field::kParagraphPropsRaw, kStylesSchemaVersion, *s.paragraph_props_raw);
    }
    if (s.run_props_raw.has_value()) {
        tlv::write_record(out, field::kRunPropsRaw, kStylesSchemaVersion, *s.run_props_raw);
    }

    return out;
}

model::StyleDefinition decode_style_definition(const std::vector<uint8_t>& payload) {
    model::StyleDefinition s;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case field::kStyleId:
                s.style_id = r.read_string();
                break;
            case field::kDisplayName:
                s.display_name = r.read_string();
                break;
            case field::kType: {
                uint8_t raw = r.read_u8();
                if (raw > static_cast<uint8_t>(model::StyleType::kLinked)) {
                    throw std::runtime_error("styles serde: unknown StyleType value " + std::to_string(raw));
                }
                s.type = static_cast<model::StyleType>(raw);
                break;
            }
            case field::kBasedOn:
                s.based_on = r.read_string();
                break;
            case field::kNextStyle:
                s.next_style = r.read_string();
                break;
            case field::kIsDefault:
                s.is_default = r.read_u8() != 0;
                break;
            case field::kQuickStyle:
                s.quick_style = r.read_u8() != 0;
                break;
            case field::kParagraphPropsRaw:
                s.paragraph_props_raw = rec.payload;
                break;
            case field::kRunPropsRaw:
                s.run_props_raw = rec.payload;
                break;
            default:
                break; // unknown field: skip
        }
    }

    return s;
}

} // namespace

std::vector<uint8_t> serialize_styles(const model::Styles& s) {
    std::vector<uint8_t> out;
    for (const auto& def : s.definitions) {
        tlv::write_record(out, record_type::kStyleDefinition, kStylesSchemaVersion,
                           encode_style_definition(def));
    }
    return out;
}

model::Styles deserialize_styles(const std::vector<uint8_t>& payload) {
    model::Styles s;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kStyleDefinition) continue; // unknown: skip
        s.definitions.push_back(decode_style_definition(rec.payload));
    }
    return s;
}

} // namespace idoc::serde
