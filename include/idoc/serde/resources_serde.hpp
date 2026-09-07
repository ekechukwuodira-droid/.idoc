#pragma once
// ResourceIndex block serde (§11) — physical container layout's
// "Block 11: Resource Index". See model/resources.hpp's SCOPE note:
// this covers index metadata only, not actual blob storage.

#include "idoc/model/resources.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kResourcesSchemaVersion = 1;

std::vector<uint8_t> serialize_resource_index(const model::ResourceIndex& idx);

// Throws std::out_of_range on truncated/malformed input.
model::ResourceIndex deserialize_resource_index(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
