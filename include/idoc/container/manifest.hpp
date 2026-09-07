#pragma once
// Manifest — §1.3. Human-inspectable JSON, source of truth if it and the
// binary Block Directory ever disagree.

#include <cstdint>
#include <string>
#include <vector>

namespace idoc {

struct ManifestBlockEntry {
    uint32_t id = 0;
    std::string type;
    uint16_t version = 0;
    uint64_t offset = 0;
    uint64_t length = 0;
};

struct Manifest {
    std::string format_version = "1.0";
    std::string created_by;
    std::string created_at;       // ISO 8601
    std::string last_modified_by;
    std::string document_id;      // stable GUID
    std::vector<ManifestBlockEntry> blocks;
    uint32_t resource_count = 0;
    uint32_t incremental_save_generation = 0;

    std::string to_json() const;
    static Manifest from_json(const std::string& json_text);
};

} // namespace idoc
