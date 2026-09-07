#pragma once
// ContainerWriter — builds a complete .idoc file byte buffer per §1.1.
//
// This stage does not implement incremental save (append + dead-space
// tracking, §1.6) or the resource blob area (§11) -- `build()` always
// produces a dense file with an empty resource area. Both are additive:
// incremental save changes how bytes are appended to an existing file,
// not the block/record shapes already defined here, and the resource
// area is a distinct block (kResourceIndex) plus a raw blob region that
// doesn't touch anything in this writer's current block loop.

#include "idoc/container/file_header.hpp"
#include "idoc/container/manifest.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace idoc {

struct PendingBlock {
    uint32_t type_id;
    uint16_t schema_version;
    std::vector<uint8_t> payload;   // uncompressed
    bool compress;
    int compression_level;
    std::string type_name;          // for the human-readable Manifest
};

class ContainerWriter {
public:
    void set_document_id(const std::string& id) { document_id_ = id; }
    void set_created_by(const std::string& app) { created_by_ = app; }
    void set_created_at(const std::string& iso8601) { created_at_ = iso8601; }
    void set_last_modified_by(const std::string& app) { last_modified_by_ = app; }

    // compression_level applies only when compress == true.
    void add_block(uint32_t type_id, const std::string& type_name, uint16_t schema_version,
                    const std::vector<uint8_t>& payload, bool compress = true,
                    int compression_level = 3);

    // Produces the full file byte buffer, including trailing CRC64.
    std::vector<uint8_t> build() const;

private:
    std::string document_id_;
    std::string created_by_ = "idoc-engine";
    std::string created_at_;
    std::string last_modified_by_ = "idoc-engine";
    std::vector<PendingBlock> blocks_;
};

} // namespace idoc
