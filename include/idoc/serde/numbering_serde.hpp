#pragma once
// NumberingDefinitions block serde (§7) — physical container layout's
// "Block 4: Numbering Definitions", holding both AbstractNum[] and
// NumberingInstance[] together.

#include "idoc/model/numbering.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kNumberingSchemaVersion = 1;

std::vector<uint8_t> serialize_numbering_definitions(const model::NumberingDefinitions& nd);

// Throws std::out_of_range on truncated/malformed input.
model::NumberingDefinitions deserialize_numbering_definitions(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
