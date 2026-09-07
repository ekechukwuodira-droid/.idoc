#pragma once
// Fields block serde (§8) — physical container layout's "Block 7: Fields".

#include "idoc/model/fields.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kFieldsSchemaVersion = 1;

std::vector<uint8_t> serialize_fields(const model::Fields& f);

// Throws std::out_of_range on truncated/malformed input.
model::Fields deserialize_fields(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
