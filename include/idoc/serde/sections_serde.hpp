#pragma once
// Sections block serde. The block payload is a list of Section records
// (repeated, like Styles' StyleDefinition), each itself a nested TLV
// field list -- same forward-compatible skip-unknown pattern throughout.

#include "idoc/model/sections.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kSectionsSchemaVersion = 1;

std::vector<uint8_t> serialize_sections(const model::Sections& s);

// Throws std::out_of_range on truncated/malformed input.
model::Sections deserialize_sections(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
