#pragma once
// LayoutCache block serde (§13) — physical container layout's
// "Block 13: Layout Cache". See model/layout_cache.hpp: this block is
// derived data, never authoritative -- a failure to deserialize this
// block should be treated as "regenerate it," never as a fatal open
// error, by whatever code eventually calls this (not built yet; that's
// the layout engine's job, not this serde layer's).

#include "idoc/model/layout_cache.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kLayoutCacheSchemaVersion = 1;

std::vector<uint8_t> serialize_layout_cache(const model::LayoutCache& lc);

// Throws std::out_of_range on truncated/malformed input. Per the note
// above, callers should catch this and regenerate rather than propagate
// it as an open failure -- this function itself just does the encoding,
// it doesn't decide open-time error policy.
model::LayoutCache deserialize_layout_cache(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
