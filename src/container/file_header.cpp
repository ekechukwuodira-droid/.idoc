#include "idoc/container/file_header.hpp"
#include "idoc/container/byteorder.hpp"

#include <stdexcept>

namespace idoc {

std::vector<uint8_t> FileHeader::serialize() const {
    std::vector<uint8_t> buf;
    buf.reserve(kFileHeaderSize);

    byteorder::write_bytes(buf, reinterpret_cast<const uint8_t*>(magic), 4);
    byteorder::write_u16(buf, format_major);
    byteorder::write_u16(buf, format_minor);
    byteorder::write_u32(buf, header_flags);
    byteorder::write_u64(buf, manifest_offset);
    byteorder::write_u64(buf, manifest_length);
    byteorder::write_u64(buf, block_directory_offset);
    byteorder::write_u64(buf, resource_area_offset);
    byteorder::write_u64(buf, file_length);
    byteorder::write_bytes(buf, reserved.data(), reserved.size());

    if (buf.size() != kFileHeaderSize) {
        throw std::logic_error("FileHeader::serialize produced unexpected size");
    }
    return buf;
}

FileHeader FileHeader::deserialize(const std::vector<uint8_t>& bytes) {
    if (bytes.size() < kFileHeaderSize) {
        throw std::out_of_range("FileHeader::deserialize: buffer shorter than fixed header size");
    }

    byteorder::Reader r(bytes);
    FileHeader h;

    auto magic_bytes = r.read_bytes(4);
    for (int i = 0; i < 4; ++i) h.magic[i] = static_cast<char>(magic_bytes[i]);

    h.format_major = r.read_u16();
    h.format_minor = r.read_u16();
    h.header_flags = r.read_u32();
    h.manifest_offset = r.read_u64();
    h.manifest_length = r.read_u64();
    h.block_directory_offset = r.read_u64();
    h.resource_area_offset = r.read_u64();
    h.file_length = r.read_u64();
    auto reserved_bytes = r.read_bytes(kReservedSize);
    for (size_t i = 0; i < kReservedSize; ++i) h.reserved[i] = reserved_bytes[i];

    if (!h.has_valid_magic()) {
        throw std::runtime_error("FileHeader::deserialize: bad magic, not an .idoc file");
    }

    return h;
}

} // namespace idoc
