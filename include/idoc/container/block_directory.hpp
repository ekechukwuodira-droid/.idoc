#pragma once
// Block Directory — §1.3. Binary duplicate of the Manifest's block offset
// table, for fast machine access without JSON parsing. The Manifest is the
// source of truth if the two ever disagree (e.g. after partial/salvage
// writes); this directory exists purely as an O(1)-seek optimization.

#include <cstdint>
#include <vector>

namespace idoc {

struct BlockDirectoryEntry {
    uint32_t id = 0;
    uint32_t type_id = 0;
    uint16_t schema_version = 0;
    uint64_t offset = 0;      // absolute offset into the file
    uint64_t length = 0;      // length of the stored (possibly compressed) bytes
    bool compressed = false;
};

struct BlockDirectory {
    std::vector<BlockDirectoryEntry> entries;

    std::vector<uint8_t> serialize() const;
    static BlockDirectory deserialize(const std::vector<uint8_t>& bytes);
};

} // namespace idoc
