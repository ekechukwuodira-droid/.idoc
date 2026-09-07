#pragma once
// Tables block serde (§9) — physical container layout's "Block 6: Tables".

#include "idoc/model/tables.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kTablesSchemaVersion = 1;

std::vector<uint8_t> serialize_tables(const model::Tables& t);

// Throws std::out_of_range on truncated/malformed input.
model::Tables deserialize_tables(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
