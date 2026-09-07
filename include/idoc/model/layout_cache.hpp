#pragma once
// Layout Cache — §13. Pure data, no I/O.
//
// This block is derived data, never authoritative (per §0 principle 4,
// as the spec's own prose states) -- a corrupt or missing LayoutCache
// must never prevent a document from opening; the engine just re-runs
// full layout and regenerates it. Nothing here changes that; this stage
// just gives the derived data a concrete, round-trippable shape.
//
// ASSUMPTION FLAGGED: `PageGeometry` is described only in prose ("per
// page: page number (resolved), section_id, content block references,
// field cache pointers"), not as a formal field list. `content block
// references` hits the same open question already flagged for
// Section.content/Cell.content (serde/paragraph_serde.hpp) -- what a
// reference into Document Content/Tables actually looks like isn't
// decided yet -- so it's reserved as opaque bytes here too, for the same
// reason. `field cache pointers` is read as a plain list of field_ids
// relevant to the page (simple ID references, same treatment as
// Run.comment_anchor_ids elsewhere).

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

struct PageGeometry {
    uint32_t page_number = 0; // resolved, i.e. the actual displayed number
    std::string section_id;
    // Reserved -- see ASSUMPTION FLAGGED note above.
    std::optional<std::vector<uint8_t>> content_block_refs_raw;
    std::vector<std::string> field_ids; // "field cache pointers" relevant to this page

    bool operator==(const PageGeometry& other) const {
        return page_number == other.page_number && section_id == other.section_id &&
               content_block_refs_raw == other.content_block_refs_raw &&
               field_ids == other.field_ids;
    }
};

// Physical container layout's "Block 13: Layout Cache".
struct LayoutCache {
    uint32_t generation = 0; // increments each full layout pass
    std::vector<PageGeometry> pages;
    std::map<std::string, std::string> field_results; // mirrors Field.cached_result, for fast bulk read

    bool operator==(const LayoutCache& other) const {
        return generation == other.generation && pages == other.pages &&
               field_results == other.field_results;
    }
};

} // namespace idoc::model
