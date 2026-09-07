#pragma once
// Theme block serde. Same nested-TLV-per-field pattern as Metadata: each
// named field (colors, fonts, and each field within those) is its own
// record, so unknown fields are skipped rather than breaking the parse
// (§1.4's forward-compatibility rule).

#include "idoc/model/theme.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kThemeSchemaVersion = 1;

std::vector<uint8_t> serialize_theme(const model::Theme& t);

// Throws std::out_of_range on truncated/malformed input.
model::Theme deserialize_theme(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
