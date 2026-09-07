#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"

namespace idoc::tlv {

void write_record(std::vector<uint8_t>& out, uint32_t type_id, uint16_t schema_version,
                   const std::vector<uint8_t>& payload) {
    byteorder::write_u32(out, type_id);
    byteorder::write_u16(out, schema_version);
    byteorder::write_u32(out, static_cast<uint32_t>(payload.size()));
    byteorder::write_bytes(out, payload);
}

std::vector<Record> parse_records(const std::vector<uint8_t>& buf) {
    std::vector<Record> records;
    byteorder::Reader r(buf);

    while (!r.at_end()) {
        // A record header is 10 bytes; if fewer remain, treat as padding/truncation
        // and stop rather than throwing -- trailing zero-padding after the last
        // record is common in append-only incremental saves.
        if (r.remaining() < kHeaderSize) break;

        Record rec;
        rec.header.type_id = r.read_u32();
        rec.header.schema_version = r.read_u16();
        rec.header.length = r.read_u32();
        rec.payload = r.read_bytes(rec.header.length); // throws if truncated mid-payload
        records.push_back(std::move(rec));
    }
    return records;
}

const Record* find_record(const std::vector<Record>& records, uint32_t type_id) {
    for (const auto& rec : records) {
        if (rec.header.type_id == type_id) return &rec;
    }
    return nullptr;
}

} // namespace idoc::tlv
