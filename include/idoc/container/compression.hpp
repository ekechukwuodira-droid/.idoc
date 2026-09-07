#pragma once
// Per-block compression — §1.5 of the spec. zstd was chosen over LZ4:
// matches LZ4 decompression speed at low levels, covers both the
// incremental-save (fast, low level) and compaction (slow, high level,
// e.g. 19) use cases with one library, and its trained-dictionary mode is
// a real future win for a format made of many small independent blocks
// (dictionary training itself is deferred -- see area notes).

#include <cstdint>
#include <vector>

namespace idoc::compression {

// Typical levels: ~3 for incremental saves, ~19 for explicit "optimize file"
// compaction passes. Level is not stored -- only the compressed bytes are;
// any level decompresses with the same call.
std::vector<uint8_t> compress(const std::vector<uint8_t>& data, int level);

// Decompresses a zstd frame produced by compress(). Throws std::runtime_error
// on a corrupt frame or if the frame's content size cannot be determined.
std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed);

} // namespace idoc::compression
