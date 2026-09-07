#pragma once
// Serde for §10's three physical blocks: Footnotes/Endnotes (Block 8),
// Comments (Block 9), Bookmarks/Hyperlinks (Block 10).

#include "idoc/model/annotations.hpp"

#include <cstdint>
#include <vector>

namespace idoc::serde {

constexpr uint16_t kAnnotationsSchemaVersion = 1;

std::vector<uint8_t> serialize_footnotes_endnotes(const model::FootnotesEndnotes& fe);
model::FootnotesEndnotes deserialize_footnotes_endnotes(const std::vector<uint8_t>& payload);

std::vector<uint8_t> serialize_comments(const model::Comments& c);
model::Comments deserialize_comments(const std::vector<uint8_t>& payload);

std::vector<uint8_t> serialize_bookmarks_hyperlinks(const model::BookmarksHyperlinks& bh);
model::BookmarksHyperlinks deserialize_bookmarks_hyperlinks(const std::vector<uint8_t>& payload);

} // namespace idoc::serde
