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
// references` now uses the reference format decided in
// model/content_ref.hpp -- the same one Section/Cell/Note/Comment
// content use. `field cache pointers` is read as a plain list of
// field_ids relevant to the page (simple ID references, same treatment
// as Run.comment_anchor_ids elsewhere).

#include "idoc/model/content_ref.hpp" // ContentRef

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace idoc::model {

struct PageGeometry {
    uint32_t page_number = 0; // resolved, i.e. the actual displayed number
    std::string section_id;
    std::vector<ContentRef> content_block_refs;
    std::vector<std::string> field_ids; // "field cache pointers" relevant to this page

    bool operator==(const PageGeometry& other) const {
        return page_number == other.page_number && section_id == other.section_id &&
               content_block_refs == other.content_block_refs &&
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

