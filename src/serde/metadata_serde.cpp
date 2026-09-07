#include "idoc/serde/metadata_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"

namespace idoc::serde {

namespace field {
// Field ids within the Metadata block. Stable once shipped -- append new
// ids for new fields, never renumber or reuse a retired one.
constexpr uint32_t kDocumentId = 1;
constexpr uint32_t kTitle = 2;
constexpr uint32_t kAuthor = 3;
constexpr uint32_t kSubject = 4;
constexpr uint32_t kKeywords = 5;
constexpr uint32_t kLanguage = 6;
constexpr uint32_t kCreatedAt = 7;
constexpr uint32_t kModifiedAt = 8;
constexpr uint32_t kApplicationVersion = 9;
constexpr uint32_t kRevisionNumber = 10;
constexpr uint32_t kCustomProperty = 11; // repeated: one record per property
} // namespace field

namespace {

std::vector<uint8_t> encode_string_field(const std::string& s) {
    std::vector<uint8_t> payload;
    byteorder::write_string(payload, s);
    return payload;
}

void write_optional_string(std::vector<uint8_t>& out, uint32_t field_id,
                            const std::optional<std::string>& value) {
    if (!value.has_value()) return;
    tlv::write_record(out, field_id, kMetadataSchemaVersion, encode_string_field(*value));
}

void write_string_field(std::vector<uint8_t>& out, uint32_t field_id, const std::string& value) {
    tlv::write_record(out, field_id, kMetadataSchemaVersion, encode_string_field(value));
}

} // namespace

std::vector<uint8_t> serialize_metadata(const model::Metadata& m) {
    std::vector<uint8_t> out;

    write_string_field(out, field::kDocumentId, m.document_id);
    write_optional_string(out, field::kTitle, m.title);
    write_optional_string(out, field::kAuthor, m.author);
    write_optional_string(out, field::kSubject, m.subject);

    {
        std::vector<uint8_t> payload;
        byteorder::write_u32(payload, static_cast<uint32_t>(m.keywords.size()));
        for (const auto& kw : m.keywords) byteorder::write_string(payload, kw);
        tlv::write_record(out, field::kKeywords, kMetadataSchemaVersion, payload);
    }

    write_string_field(out, field::kLanguage, m.language);
    write_string_field(out, field::kCreatedAt, m.created_at);
    write_string_field(out, field::kModifiedAt, m.modified_at);
    write_string_field(out, field::kApplicationVersion, m.application_version);

    {
        std::vector<uint8_t> payload;
        byteorder::write_u32(payload, m.revision_number);
        tlv::write_record(out, field::kRevisionNumber, kMetadataSchemaVersion, payload);
    }

    for (const auto& prop : m.custom) {
        std::vector<uint8_t> payload;
        byteorder::write_string(payload, prop.key);
        byteorder::write_string(payload, prop.value);
        tlv::write_record(out, field::kCustomProperty, kMetadataSchemaVersion, payload);
    }

    return out;
}

model::Metadata deserialize_metadata(const std::vector<uint8_t>& payload) {
    model::Metadata m;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case field::kDocumentId:
                m.document_id = r.read_string();
                break;
            case field::kTitle:
                m.title = r.read_string();
                break;
            case field::kAuthor:
                m.author = r.read_string();
                break;
            case field::kSubject:
                m.subject = r.read_string();
                break;
            case field::kKeywords: {
                uint32_t count = r.read_u32();
                m.keywords.reserve(count);
                for (uint32_t i = 0; i < count; ++i) m.keywords.push_back(r.read_string());
                break;
            }
            case field::kLanguage:
                m.language = r.read_string();
                break;
            case field::kCreatedAt:
                m.created_at = r.read_string();
                break;
            case field::kModifiedAt:
                m.modified_at = r.read_string();
                break;
            case field::kApplicationVersion:
                m.application_version = r.read_string();
                break;
            case field::kRevisionNumber:
                m.revision_number = r.read_u32();
                break;
            case field::kCustomProperty: {
                model::CustomProperty prop;
                prop.key = r.read_string();
                prop.value = r.read_string();
                m.custom.push_back(std::move(prop));
                break;
            }
            default:
                // Unknown field id: skip. rec.payload was already consumed
                // as opaque bytes by parse_records via its length prefix,
                // so there is nothing further to do here.
                break;
        }
    }

    return m;
}

} // namespace idoc::serde
