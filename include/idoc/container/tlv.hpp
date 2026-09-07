#pragma once
// TLV encoding — §1.4 of the spec.
//
// BlockHeader is the shape used both for top-level container blocks and,
// recursively, for records nested inside a block's payload. Unknown
// records are skipped by length, never by name -- this is what lets an
// older reader open a newer file and silently drop fields it doesn't
// know, while the caller (ContainerReader / block serde) is responsible
// for mirroring the raw bytes of anything it skips into the
// Preserved-Unknown Store (§12) so nothing is lost. This module only
// does the skip; the preservation bookkeeping lives one layer up.

#include <cstdint>
#include <vector>

namespace idoc::tlv {

constexpr size_t kHeaderSize = 4 + 2 + 4; // type_id(u32) + schema_version(u16) + length(u32)

struct BlockHeader {
    uint32_t type_id = 0;
    uint16_t schema_version = 0;
    uint32_t length = 0; // bytes following this header, i.e. payload length
};

// A single decoded TLV record: header plus its raw (still-encoded) payload bytes.
struct Record {
    BlockHeader header;
    std::vector<uint8_t> payload;
};

// Appends a single TLV record (header + payload) to `out`.
void write_record(std::vector<uint8_t>& out, uint32_t type_id, uint16_t schema_version,
                   const std::vector<uint8_t>& payload);

// Parses every top-level record found in `buf` (does not recurse into
// payloads -- callers recurse manually when a payload is itself a list of
// nested records). Throws std::out_of_range on truncated/malformed input;
// callers doing best-effort/salvage parsing should catch per-buffer.
std::vector<Record> parse_records(const std::vector<uint8_t>& buf);

// Convenience: find the first record of a given type_id, or nullptr if absent.
// Returned pointer aliases into `records` and is valid only as long as it is.
const Record* find_record(const std::vector<Record>& records, uint32_t type_id);

} // namespace idoc::tlv
