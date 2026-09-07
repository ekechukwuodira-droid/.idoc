#pragma once
// PreservedUnknown block serde (§12) — physical container layout's
// "Block 12: Preserved-Unknown Store".

#include "idoc/model/preserved_unknown.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kPreservedUnknownSchemaVersion = 1;

std::vector<uint8_t> serialize_preserved_unknown(const model::PreservedUnknown& pu);

// Throws std::out_of_range on truncated/malformed input.
model::PreservedUnknown deserialize_preserved_unknown(const std::vector<uint8_t>& payload);

// Builds a PreservedNode with checksum computed from raw_payload
// (CRC64 -- see the ASSUMPTION FLAGGED note in model/preserved_unknown.hpp).
// Convenience for callers constructing nodes; not required for round-trip.
model::PreservedNode make_preserved_node(std::string anchor_node_id, std::string origin_format,
                                          std::string origin_path, std::vector<uint8_t> raw_payload);

// Returns true if node.checksum matches a freshly computed CRC64 of
// node.raw_payload -- i.e. the fragment wasn't corrupted independent of
// whether its anchor node still exists.
bool verify_preserved_node_checksum(const model::PreservedNode& node);

} // namespace idoc::serde
