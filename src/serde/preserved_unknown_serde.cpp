#include "idoc/serde/preserved_unknown_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/container/crc64.hpp"

namespace idoc::serde {

namespace record_type {
constexpr uint32_t kNode = 1; // only repeated record type at the block's top level
} // namespace record_type

namespace node_field {
constexpr uint32_t kAnchorNodeId = 1;
constexpr uint32_t kOriginFormat = 2;
constexpr uint32_t kOriginPath = 3;
constexpr uint32_t kRawPayload = 4;
constexpr uint32_t kChecksum = 5;
} // namespace node_field

namespace {

std::vector<uint8_t> encode_node(const model::PreservedNode& node) {
    std::vector<uint8_t> out;

    tlv::write_record(out, node_field::kAnchorNodeId, kPreservedUnknownSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, node.anchor_node_id); return p; }());
    tlv::write_record(out, node_field::kOriginFormat, kPreservedUnknownSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, node.origin_format); return p; }());
    tlv::write_record(out, node_field::kOriginPath, kPreservedUnknownSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, node.origin_path); return p; }());
    tlv::write_record(out, node_field::kRawPayload, kPreservedUnknownSchemaVersion, node.raw_payload);
    tlv::write_record(out, node_field::kChecksum, kPreservedUnknownSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u64(p, node.checksum); return p; }());

    return out;
}

model::PreservedNode decode_node(const std::vector<uint8_t>& payload) {
    model::PreservedNode node;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case node_field::kAnchorNodeId:
                node.anchor_node_id = r.read_string();
                break;
            case node_field::kOriginFormat:
                node.origin_format = r.read_string();
                break;
            case node_field::kOriginPath:
                node.origin_path = r.read_string();
                break;
            case node_field::kRawPayload:
                node.raw_payload = rec.payload;
                break;
            case node_field::kChecksum:
                node.checksum = r.read_u64();
                break;
            default:
                break; // unknown field: skip
        }
    }
    return node;
}

} // namespace

std::vector<uint8_t> serialize_preserved_unknown(const model::PreservedUnknown& pu) {
    std::vector<uint8_t> out;
    for (const auto& node : pu.nodes) {
        tlv::write_record(out, record_type::kNode, kPreservedUnknownSchemaVersion, encode_node(node));
    }
    return out;
}

model::PreservedUnknown deserialize_preserved_unknown(const std::vector<uint8_t>& payload) {
    model::PreservedUnknown pu;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kNode) continue; // unknown top-level record: skip
        pu.nodes.push_back(decode_node(rec.payload));
    }
    return pu;
}

model::PreservedNode make_preserved_node(std::string anchor_node_id, std::string origin_format,
                                          std::string origin_path, std::vector<uint8_t> raw_payload) {
    model::PreservedNode node;
    node.checksum = crc64(raw_payload);
    node.anchor_node_id = std::move(anchor_node_id);
    node.origin_format = std::move(origin_format);
    node.origin_path = std::move(origin_path);
    node.raw_payload = std::move(raw_payload);
    return node;
}

bool verify_preserved_node_checksum(const model::PreservedNode& node) {
    return crc64(node.raw_payload) == node.checksum;
}

} // namespace idoc::serde
