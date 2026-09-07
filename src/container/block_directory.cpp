#include "idoc/container/block_directory.hpp"
#include "idoc/container/byteorder.hpp"

namespace idoc {

namespace {
// id(u32) + type_id(u32) + schema_version(u16) + offset(u64) + length(u64) + compressed(u8)
constexpr size_t kEntrySize = 4 + 4 + 2 + 8 + 8 + 1;
} // namespace

std::vector<uint8_t> BlockDirectory::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(4 + entries.size() * kEntrySize);

    byteorder::write_u32(buf, static_cast<uint32_t>(entries.size()));
    for (const auto& e : entries) {
        byteorder::write_u32(buf, e.id);
        byteorder::write_u32(buf, e.type_id);
        byteorder::write_u16(buf, e.schema_version);
        byteorder::write_u64(buf, e.offset);
        byteorder::write_u64(buf, e.length);
        byteorder::write_u8(buf, e.compressed ? 1 : 0);
    }
    return buf;
}

BlockDirectory BlockDirectory::deserialize(const std::vector<uint8_t>& bytes) {
    byteorder::Reader r(bytes);
    BlockDirectory dir;

    uint32_t count = r.read_u32();
    dir.entries.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        BlockDirectoryEntry e;
        e.id = r.read_u32();
        e.type_id = r.read_u32();
        e.schema_version = r.read_u16();
        e.offset = r.read_u64();
        e.length = r.read_u64();
        e.compressed = r.read_u8() != 0;
        dir.entries.push_back(e);
    }
    return dir;
}

} // namespace idoc
