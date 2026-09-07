#include "idoc/container/container_reader.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/container/compression.hpp"
#include "idoc/container/crc64.hpp"
#include "idoc/container/tlv.hpp"

#include <stdexcept>

namespace idoc {

ContainerReader ContainerReader::open(const std::vector<uint8_t>& file_bytes) {
    if (file_bytes.size() < kFileHeaderSize + 8) {
        throw std::out_of_range("ContainerReader::open: file too short to contain a header and checksum");
    }

    // Verify trailing CRC64 over everything preceding it.
    size_t checksum_pos = file_bytes.size() - 8;
    std::vector<uint8_t> body(file_bytes.begin(), file_bytes.begin() + checksum_pos);
    byteorder::Reader checksum_reader(file_bytes, checksum_pos);
    uint64_t stored_checksum = checksum_reader.read_u64();
    uint64_t computed_checksum = crc64(body);
    if (stored_checksum != computed_checksum) {
        throw std::runtime_error("ContainerReader::open: trailing checksum mismatch, file is corrupt or truncated");
    }

    std::vector<uint8_t> header_bytes(file_bytes.begin(), file_bytes.begin() + kFileHeaderSize);
    FileHeader header = FileHeader::deserialize(header_bytes);

    if (header.format_major != kFormatMajor) {
        throw std::runtime_error("ContainerReader::open: unsupported format_major version "
                                  "(reader refuses to open per spec §1.2; salvage mode not yet implemented)");
    }

    if (header.manifest_offset + header.manifest_length > file_bytes.size()) {
        throw std::out_of_range("ContainerReader::open: manifest extends past end of file");
    }
    std::string manifest_json(
        reinterpret_cast<const char*>(file_bytes.data() + header.manifest_offset),
        header.manifest_length);
    Manifest manifest = Manifest::from_json(manifest_json);

    if (header.block_directory_offset > file_bytes.size() ||
        header.resource_area_offset > file_bytes.size() ||
        header.resource_area_offset < header.block_directory_offset) {
        throw std::out_of_range("ContainerReader::open: block directory bounds invalid");
    }
    std::vector<uint8_t> directory_bytes(
        file_bytes.begin() + header.block_directory_offset,
        file_bytes.begin() + header.resource_area_offset);
    BlockDirectory directory = BlockDirectory::deserialize(directory_bytes);

    ContainerReader reader;
    reader.header_ = header;
    reader.manifest_ = std::move(manifest);
    reader.directory_ = std::move(directory);
    reader.file_bytes_ = file_bytes;
    return reader;
}

std::optional<std::vector<uint8_t>> ContainerReader::read_block(uint32_t type_id) const {
    for (const auto& entry : directory_.entries) {
        if (entry.type_id != type_id) continue;

        if (entry.offset + entry.length > file_bytes_.size()) {
            throw std::out_of_range("ContainerReader::read_block: block extends past end of file");
        }
        std::vector<uint8_t> record_bytes(
            file_bytes_.begin() + entry.offset,
            file_bytes_.begin() + entry.offset + entry.length);

        auto records = tlv::parse_records(record_bytes);
        if (records.empty()) {
            throw std::runtime_error("ContainerReader::read_block: block directory points at an empty/malformed record");
        }
        const std::vector<uint8_t>& stored_payload = records.front().payload;

        return entry.compressed ? compression::decompress(stored_payload) : stored_payload;
    }
    return std::nullopt;
}

} // namespace idoc
