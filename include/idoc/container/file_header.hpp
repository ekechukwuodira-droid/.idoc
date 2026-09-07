#pragma once
// FileHeader — §1.2 of the .idoc spec.
//
// NOTE ON SPEC DISCREPANCY: the spec's struct listing (magic[4] + 2*u16 +
// u32 + 5*u64 + reserved[16]) sums to 68 bytes, not the "fixed 64 bytes"
// stated in prose. We've resolved this by shrinking `reserved` to 12 bytes
// (68 - 4 = 64) rather than silently picking 68 and calling it 64. Flagging
// this so it can be corrected in the next spec draft -- if the intent was
// actually 68, this is a one-line change (kReservedSize below) with no
// other code affected, since nothing yet depends on the reserved region.

#include <array>
#include <cstdint>
#include <vector>

namespace idoc {

constexpr uint32_t kFormatMajor = 1;
constexpr uint32_t kFormatMinor = 0;

constexpr size_t kReservedSize = 12; // see discrepancy note above
constexpr size_t kFileHeaderSize = 4 + 2 + 2 + 4 + 8 + 8 + 8 + 8 + 8 + kReservedSize; // == 64

enum HeaderFlagBit : uint32_t {
    kFlagEncrypted = 0,                 // bit 0
    kFlagIncrementalSavePending = 1,    // bit 1
};

struct FileHeader {
    char magic[4] = {'I', 'D', 'O', 'C'};
    uint16_t format_major = kFormatMajor;
    uint16_t format_minor = kFormatMinor;
    uint32_t header_flags = 0;
    uint64_t manifest_offset = 0;
    uint64_t manifest_length = 0;
    uint64_t block_directory_offset = 0;
    uint64_t resource_area_offset = 0;
    uint64_t file_length = 0;
    std::array<uint8_t, kReservedSize> reserved{};

    bool has_flag(HeaderFlagBit bit) const {
        return (header_flags & (1u << static_cast<uint32_t>(bit))) != 0;
    }
    void set_flag(HeaderFlagBit bit, bool value) {
        if (value) {
            header_flags |= (1u << static_cast<uint32_t>(bit));
        } else {
            header_flags &= ~(1u << static_cast<uint32_t>(bit));
        }
    }

    bool has_valid_magic() const {
        return magic[0] == 'I' && magic[1] == 'D' && magic[2] == 'O' && magic[3] == 'C';
    }

    std::vector<uint8_t> serialize() const;

    // Throws std::runtime_error on bad magic, std::out_of_range on truncated input.
    static FileHeader deserialize(const std::vector<uint8_t>& bytes);
};

} // namespace idoc
