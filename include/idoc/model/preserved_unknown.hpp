#pragma once
// Preserved-Unknown Store — §12, the DOCX-safety mechanism. Pure data,
// no I/O.
//
// IMPORTANT DISTINCTION: this is NOT the same mechanism as this
// codebase's "unknown TLV field is skipped" forward-compatibility
// pattern used throughout every other block's serde. That pattern
// handles schema evolution *within* .idoc across engine versions (an
// older reader silently drops a field a newer writer added). This block
// handles round-tripping data from a *foreign* format (DOCX/OOXML) that
// has no native .idoc representation at all -- e.g. an OOXML extension
// element the importer doesn't understand. The two are conceptually
// separate even though both are "don't lose data you can't interpret."
// This stage builds the model/serde for the block only; it does not wire
// the two mechanisms together.
//
// ASSUMPTION FLAGGED: `Checksum` is used but never defined as a type
// anywhere in the spec. Reusing CRC64 (idoc::crc64) here, since it's the
// only checksum algorithm the spec defines anywhere (§1.1's whole-file
// trailing checksum) -- introducing a second, different hash algorithm
// for this one field would be an unforced complication.

#include <cstdint>
#include <string>
#include <vector>

namespace idoc::model {

struct PreservedNode {
    std::string anchor_node_id; // a paragraph_id/run_id/etc., or "document" for document-level
    std::string origin_format;  // e.g. "docx-ooxml"
    std::string origin_path;    // XPath-like locator in the original XML, for diagnostics
    std::vector<uint8_t> raw_payload; // original serialized fragment, untouched
    uint64_t checksum = 0;            // CRC64 of raw_payload -- see ASSUMPTION FLAGGED note above

    bool operator==(const PreservedNode& other) const {
        return anchor_node_id == other.anchor_node_id &&
               origin_format == other.origin_format &&
               origin_path == other.origin_path &&
               raw_payload == other.raw_payload &&
               checksum == other.checksum;
    }
};

// Physical container layout's "Block 12: Preserved-Unknown Store".
struct PreservedUnknown {
    std::vector<PreservedNode> nodes;

    bool operator==(const PreservedUnknown& other) const {
        return nodes == other.nodes;
    }
};

} // namespace idoc::model
