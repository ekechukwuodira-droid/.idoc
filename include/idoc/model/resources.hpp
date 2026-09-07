#pragma once
// Resources — §11. Pure data, no I/O.
//
// SCOPE: this stage models and serializes the ResourceIndex/ResourceEntry
// metadata fully -- it does NOT implement actually writing resource blob
// bytes into the container's resource area. `blob_offset`/`blob_length`
// are stored as plain fields (so the index round-trips correctly and can
// describe where blobs *would* live), but ContainerWriter still always
// emits an empty resource area (unchanged since the very first stage).
// Wiring real blob storage is separate, additional work: an API on
// ContainerWriter to append raw resource bytes and report back their
// offset/length, and a ContainerReader method to read a blob by
// resource_id. That's follow-up work, not part of modeling the index.
//
// ASSUMPTION FLAGGED: `MimeType` is used but never defined as a distinct
// type anywhere in the spec (unlike, say, `NumberFormat`, which is at
// least given an enumerated value set). It's unambiguous enough not to
// warrant a closed enum of its own -- MIME types are an open, growing set
// ("image/png", "image/jpeg", "font/ttf", ...) -- so it's modeled as a
// plain string alias.
//
// ASSUMPTION FLAGGED: `sha256` is stored as a lowercase hex string (64
// chars) rather than raw 32 bytes. The spec doesn't say either way; hex
// keeps the Manifest-adjacent index human-inspectable, consistent with
// this format's general preference for JSON/text where it costs little.

#include "idoc/model/sections.hpp" // Size2D

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

using MimeType = std::string; // see ASSUMPTION FLAGGED note above

struct ResourceEntry {
    std::string resource_id;
    std::optional<std::string> original_filename;
    MimeType mime_type;
    uint64_t blob_offset = 0; // into the container's resource area -- see SCOPE note above
    uint64_t blob_length = 0;
    std::optional<Size2D> natural_size; // for images -- pre-decode dimensions
    std::string sha256;                 // hex string, see ASSUMPTION FLAGGED note above

    bool operator==(const ResourceEntry& other) const {
        return resource_id == other.resource_id &&
               original_filename == other.original_filename &&
               mime_type == other.mime_type && blob_offset == other.blob_offset &&
               blob_length == other.blob_length && natural_size == other.natural_size &&
               sha256 == other.sha256;
    }
};

// Physical container layout's "Block 11: Resource Index".
struct ResourceIndex {
    std::vector<ResourceEntry> entries;

    bool operator==(const ResourceIndex& other) const {
        return entries == other.entries;
    }
};

} // namespace idoc::model
