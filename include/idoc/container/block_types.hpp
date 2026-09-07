#pragma once
// Block type IDs — §1.1's numbered block list. These are the `type_id`
// values used in Manifest entries, the Block Directory, and TLV block
// headers. Only kMetadata is implemented by serde in this stage; the rest
// are reserved so later stages don't have to renumber anything already
// shipped.

#include <cstdint>

namespace idoc::block_type {

constexpr uint32_t kMetadata = 0;
constexpr uint32_t kTheme = 1;
constexpr uint32_t kStyles = 2;
constexpr uint32_t kSections = 3;
constexpr uint32_t kNumberingDefinitions = 4;
constexpr uint32_t kDocumentContent = 5;
constexpr uint32_t kTables = 6;
constexpr uint32_t kFields = 7;
constexpr uint32_t kFootnotesEndnotes = 8;
constexpr uint32_t kComments = 9;
constexpr uint32_t kBookmarksHyperlinks = 10;
constexpr uint32_t kResourceIndex = 11;
constexpr uint32_t kPreservedUnknown = 12;
constexpr uint32_t kLayoutCache = 13;

} // namespace idoc::block_type
