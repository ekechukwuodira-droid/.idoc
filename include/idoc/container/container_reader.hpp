#pragma once
// ContainerReader — opens a .idoc file byte buffer, verifies the trailing
// checksum, and exposes decompressed block payloads by type_id.
//
// Salvage mode (§15) is NOT implemented here yet -- `open()` throws on any
// checksum or parse failure. A `open_best_effort()`-style entry point that
// tries the Manifest, then each block independently, belongs in a later
// stage once more block types exist to make partial recovery meaningful.

#include "idoc/container/block_directory.hpp"
#include "idoc/container/file_header.hpp"
#include "idoc/container/manifest.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace idoc {

class ContainerReader {
public:
    // Throws std::runtime_error on bad magic or checksum mismatch,
    // std::out_of_range on truncated input.
    static ContainerReader open(const std::vector<uint8_t>& file_bytes);

    const FileHeader& header() const { return header_; }
    const Manifest& manifest() const { return manifest_; }

    // Returns the decompressed payload for the first block matching
    // type_id, or std::nullopt if no such block exists.
    std::optional<std::vector<uint8_t>> read_block(uint32_t type_id) const;

private:
    FileHeader header_;
    Manifest manifest_;
    BlockDirectory directory_;
    std::vector<uint8_t> file_bytes_; // retained for on-demand block reads
};

} // namespace idoc
