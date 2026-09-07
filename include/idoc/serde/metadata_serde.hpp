#pragma once
// Metadata block serde. Each Metadata field is written as its own nested
// TLV record inside the block payload (field id below), so a future
// engine version can add a field without bumping schema_version for
// existing fields, and an older reader silently skips fields it doesn't
// recognize (§1.4's TLV rule) instead of failing to parse.
//
// Whatever is skipped here is NOT yet mirrored into the Preserved-Unknown
// Store (§12) -- that wiring lands with the PreservedUnknown block itself
// in a later stage. For now, unknown fields are dropped on re-save, which
// is safe only because Metadata has no import path yet (nothing external
// writes into it that we'd need to preserve).

#include "idoc/model/metadata.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kMetadataSchemaVersion = 1;

std::vector<uint8_t> serialize_metadata(const model::Metadata& m);

// Throws std::out_of_range on truncated/malformed input.
model::Metadata deserialize_metadata(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
