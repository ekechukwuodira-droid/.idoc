#include "idoc/container/container_writer.hpp"
#include "idoc/container/block_directory.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/container/compression.hpp"
#include "idoc/container/crc64.hpp"
#include "idoc/container/tlv.hpp"

#include <stdexcept>

namespace idoc {

void ContainerWriter::add_block(uint32_t type_id, const std::string& type_name,
                                 uint16_t schema_version, const std::vector<uint8_t>& payload,
                                 bool compress, int compression_level) {
    PendingBlock b;
    b.type_id = type_id;
    b.type_name = type_name;
    b.schema_version = schema_version;
    b.payload = payload;
    b.compress = compress;
    b.compression_level = compression_level;
    blocks_.push_back(std::move(b));
}

std::vector<uint8_t> ContainerWriter::build() const {
    // 1. Encode each block's TLV record (header + payload), optionally
    //    compressing the *payload* before wrapping it in the TLV header --
    //    the TLV length field always describes the stored (possibly
    //    compressed) bytes, since that's what a reader needs to seek past
    //    this block without decompressing it.
    struct EncodedBlock {
        uint32_t type_id;
        uint16_t schema_version;
        std::string type_name;
        std::vector<uint8_t> record_bytes; // full TLV record: header + stored payload
        uint64_t length = 0;
        bool compressed = false;
    };

    std::vector<EncodedBlock> encoded;
    encoded.reserve(blocks_.size());

    for (const auto& b : blocks_) {
        std::vector<uint8_t> stored_payload =
            b.compress ? compression::compress(b.payload, b.compression_level) : b.payload;

        EncodedBlock eb;
        eb.type_id = b.type_id;
        eb.schema_version = b.schema_version;
        eb.type_name = b.type_name;
        eb.compressed = b.compress;

        tlv::write_record(eb.record_bytes, b.type_id, b.schema_version, stored_payload);
        eb.length = eb.record_bytes.size();
        encoded.push_back(std::move(eb));
    }

    // 2. Lay out the file: header, manifest, block directory, blocks, then
    //    an (empty, in this stage) resource area.
    //
    //    This is a fixed-point problem: block offsets depend on the
    //    manifest's serialized size, but the manifest's serialized size
    //    depends on the block offsets it embeds (larger offset numbers ->
    //    more JSON digits -> larger manifest -> larger offsets...). We
    //    iterate until the manifest size stops changing, then do exactly
    //    one more serialization pass with those final offsets -- and reuse
    //    that exact same string for both the header's manifest_length and
    //    the bytes actually written to the file, so there is no seam where
    //    a later re-serialization could silently drift out of sync with
    //    what was already used to compute offsets (this is what broke
    //    the earlier two-pass version: offsets were computed from one
    //    serialization's size, but a *different*, later serialization's
    //    bytes were the ones actually written).
    Manifest manifest;
    manifest.format_version = "1.0";
    manifest.created_by = created_by_;
    manifest.created_at = created_at_;
    manifest.last_modified_by = last_modified_by_;
    manifest.document_id = document_id_;
    manifest.resource_count = 0;
    manifest.incremental_save_generation = 0;

    uint64_t running_id = 0;
    for (const auto& eb : encoded) {
        ManifestBlockEntry entry;
        entry.id = static_cast<uint32_t>(running_id++);
        entry.type = eb.type_name;
        entry.version = eb.schema_version;
        entry.offset = 0; // placeholder, filled in by the fixed-point loop below
        entry.length = eb.length;
        manifest.blocks.push_back(entry);
    }

    BlockDirectory directory;
    directory.entries.reserve(encoded.size());
    for (size_t i = 0; i < encoded.size(); ++i) {
        BlockDirectoryEntry de;
        de.id = static_cast<uint32_t>(i);
        de.type_id = encoded[i].type_id;
        de.schema_version = encoded[i].schema_version;
        de.offset = 0; // placeholder, filled in by the fixed-point loop below
        de.length = encoded[i].length;
        de.compressed = encoded[i].compressed;
        directory.entries.push_back(de);
    }

    constexpr uint64_t kManifestOffset = kFileHeaderSize;
    std::string manifest_json = manifest.to_json();
    std::vector<uint8_t> directory_bytes = directory.serialize(); // fixed size regardless of offset values

    constexpr int kMaxIterations = 8;
    bool converged = false;
    for (int iter = 0; iter < kMaxIterations; ++iter) {
        uint64_t directory_offset = kManifestOffset + manifest_json.size();
        uint64_t blocks_start = directory_offset + directory_bytes.size();

        uint64_t offset = blocks_start;
        bool any_offset_changed = false;
        for (size_t i = 0; i < encoded.size(); ++i) {
            if (manifest.blocks[i].offset != offset) any_offset_changed = true;
            manifest.blocks[i].offset = offset;
            directory.entries[i].offset = offset;
            offset += encoded[i].length;
        }

        if (!any_offset_changed) {
            converged = true;
            break;
        }

        // Offsets changed -> manifest content changed -> re-serialize
        // before checking convergence again. directory_bytes never
        // changes size (fixed-width binary), so it doesn't need
        // re-serializing, only re-populating (done via the loop above,
        // will be re-serialized to bytes just before writing the file).
        manifest_json = manifest.to_json();
    }

    if (!converged) {
        throw std::logic_error("ContainerWriter::build: manifest offset layout did not converge");
    }

    // Final, authoritative values -- these are the exact bytes written below.
    directory_bytes = directory.serialize();
    uint64_t manifest_offset = kManifestOffset;
    uint64_t manifest_length = manifest_json.size();
    uint64_t directory_offset = manifest_offset + manifest_length;
    uint64_t blocks_start = directory_offset + directory_bytes.size();
    uint64_t resource_area_offset = blocks_start;
    for (const auto& eb : encoded) resource_area_offset += eb.length;

    FileHeader header;
    header.manifest_offset = manifest_offset;
    header.manifest_length = manifest_length;
    header.block_directory_offset = directory_offset;
    header.resource_area_offset = resource_area_offset;
    header.file_length = 0; // filled in after full assembly, below

    std::vector<uint8_t> file;
    file.reserve(resource_area_offset + 8 /* checksum */);

    auto header_bytes = header.serialize();
    byteorder::write_bytes(file, header_bytes);
    byteorder::write_bytes(file, reinterpret_cast<const uint8_t*>(manifest_json.data()),
                            manifest_json.size());
    byteorder::write_bytes(file, directory_bytes);
    for (const auto& eb : encoded) {
        byteorder::write_bytes(file, eb.record_bytes);
    }
    // Resource area intentionally empty in this stage.

    // Patch file_length now that the full pre-checksum length is known.
    header.file_length = file.size() + 8; // + trailing CRC64
    auto final_header_bytes = header.serialize();
    for (size_t i = 0; i < final_header_bytes.size(); ++i) {
        file[i] = final_header_bytes[i];
    }

    uint64_t checksum = crc64(file);
    byteorder::write_u64(file, checksum);

    return file;
}

} // namespace idoc

