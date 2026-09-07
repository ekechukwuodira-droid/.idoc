#include "idoc/serde/resources_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

namespace idoc::serde {

namespace record_type {
constexpr uint32_t kEntry = 1; // only repeated record type at the block's top level
} // namespace record_type

namespace entry_field {
constexpr uint32_t kResourceId = 1;
constexpr uint32_t kOriginalFilename = 2;
constexpr uint32_t kMimeType = 3;
constexpr uint32_t kBlobOffset = 4;
constexpr uint32_t kBlobLength = 5;
constexpr uint32_t kNaturalSize = 6;
constexpr uint32_t kSha256 = 7;
} // namespace entry_field

namespace {

std::vector<uint8_t> encode_entry(const model::ResourceEntry& e) {
    std::vector<uint8_t> out;

    tlv::write_record(out, entry_field::kResourceId, kResourcesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, e.resource_id); return p; }());

    if (e.original_filename.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, *e.original_filename);
        tlv::write_record(out, entry_field::kOriginalFilename, kResourcesSchemaVersion, p);
    }

    tlv::write_record(out, entry_field::kMimeType, kResourcesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, e.mime_type); return p; }());
    tlv::write_record(out, entry_field::kBlobOffset, kResourcesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u64(p, e.blob_offset); return p; }());
    tlv::write_record(out, entry_field::kBlobLength, kResourcesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u64(p, e.blob_length); return p; }());

    if (e.natural_size.has_value()) {
        tlv::write_record(out, entry_field::kNaturalSize, kResourcesSchemaVersion,
                           common::encode_size2d(*e.natural_size));
    }

    tlv::write_record(out, entry_field::kSha256, kResourcesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, e.sha256); return p; }());

    return out;
}

model::ResourceEntry decode_entry(const std::vector<uint8_t>& payload) {
    model::ResourceEntry e;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case entry_field::kResourceId:
                e.resource_id = r.read_string();
                break;
            case entry_field::kOriginalFilename:
                e.original_filename = r.read_string();
                break;
            case entry_field::kMimeType:
                e.mime_type = r.read_string();
                break;
            case entry_field::kBlobOffset:
                e.blob_offset = r.read_u64();
                break;
            case entry_field::kBlobLength:
                e.blob_length = r.read_u64();
                break;
            case entry_field::kNaturalSize:
                e.natural_size = common::decode_size2d(rec.payload);
                break;
            case entry_field::kSha256:
                e.sha256 = r.read_string();
                break;
            default:
                break; // unknown field: skip
        }
    }
    return e;
}

} // namespace

std::vector<uint8_t> serialize_resource_index(const model::ResourceIndex& idx) {
    std::vector<uint8_t> out;
    for (const auto& e : idx.entries) {
        tlv::write_record(out, record_type::kEntry, kResourcesSchemaVersion, encode_entry(e));
    }
    return out;
}

model::ResourceIndex deserialize_resource_index(const std::vector<uint8_t>& payload) {
    model::ResourceIndex idx;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kEntry) continue; // unknown top-level record: skip
        idx.entries.push_back(decode_entry(rec.payload));
    }
    return idx;
}

} // namespace idoc::serde
