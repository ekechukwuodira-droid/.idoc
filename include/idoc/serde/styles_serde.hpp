#pragma once
// Styles block serde. The block payload is a list of StyleDefinition
// records (repeated, like Metadata's CustomProperty), each itself a
// nested TLV field list -- same forward-compatible skip-unknown pattern
// throughout.

#include "idoc/model/styles.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kStylesSchemaVersion = 1;

std::vector<uint8_t> serialize_styles(const model::Styles& s);

// Throws std::out_of_range on truncated/malformed input.
model::Styles deserialize_styles(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
