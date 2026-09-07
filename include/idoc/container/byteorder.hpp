#pragma once
// Portable little-endian encode/decode helpers.
//
// The .idoc container format specifies fixed-width little-endian integers
// for every header/TLV field. We do not rely on host byte order or struct
// memcpy tricks (which would break on big-endian hosts and are fragile
// under compiler padding), so every multi-byte field is packed/unpacked
// byte-by-byte here.

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace idoc::byteorder {

inline void write_u8(std::vector<uint8_t>& buf, uint8_t v) {
    buf.push_back(v);
}

inline void write_u16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline void write_u32(std::vector<uint8_t>& buf, uint32_t v) {
    for (int i = 0; i < 4; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

inline void write_u64(std::vector<uint8_t>& buf, uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

inline void write_bytes(std::vector<uint8_t>& buf, const uint8_t* data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

inline void write_bytes(std::vector<uint8_t>& buf, const std::vector<uint8_t>& data) {
    buf.insert(buf.end(), data.begin(), data.end());
}

// Length-prefixed (uint32 length) UTF-8 string, used inside TLV record payloads.
inline void write_string(std::vector<uint8_t>& buf, const std::string& s) {
    write_u32(buf, static_cast<uint32_t>(s.size()));
    write_bytes(buf, reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

// IEEE-754 single precision, written as raw bits via little-endian u32.
// Assumes the host uses IEEE-754 floats (true for every platform this
// project targets -- Windows/Linux/macOS on x86_64/arm64).
inline void write_f32(std::vector<uint8_t>& buf, float v) {
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(v), "float must be 4 bytes");
    std::memcpy(&bits, &v, sizeof(bits));
    write_u32(buf, bits);
}

// Cursor-based reader over a byte buffer. Every read bounds-checks and
// throws std::out_of_range on truncated/corrupt input rather than reading
// past the end -- callers (e.g. salvage mode) are expected to catch this
// per-block rather than let it propagate and abort the whole open.
struct Reader {
    const uint8_t* data;
    size_t size;
    size_t pos = 0;

    Reader(const uint8_t* d, size_t s, size_t start_pos = 0)
        : data(d), size(s), pos(start_pos) {}

    explicit Reader(const std::vector<uint8_t>& v, size_t start_pos = 0)
        : data(v.data()), size(v.size()), pos(start_pos) {}

    void need(size_t n) const {
        if (pos + n > size || pos + n < pos) {
            throw std::out_of_range("idoc::byteorder::Reader: read past end of buffer");
        }
    }

    bool at_end() const { return pos >= size; }
    size_t remaining() const { return size - pos; }

    uint8_t read_u8() {
        need(1);
        return data[pos++];
    }

    uint16_t read_u16() {
        need(2);
        uint16_t v = static_cast<uint16_t>(data[pos]) |
                     (static_cast<uint16_t>(data[pos + 1]) << 8);
        pos += 2;
        return v;
    }

    uint32_t read_u32() {
        need(4);
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v |= static_cast<uint32_t>(data[pos + i]) << (8 * i);
        }
        pos += 4;
        return v;
    }

    uint64_t read_u64() {
        need(8);
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v |= static_cast<uint64_t>(data[pos + i]) << (8 * i);
        }
        pos += 8;
        return v;
    }

    std::vector<uint8_t> read_bytes(size_t len) {
        need(len);
        std::vector<uint8_t> out(data + pos, data + pos + len);
        pos += len;
        return out;
    }

    std::string read_string() {
        uint32_t len = read_u32();
        need(len);
        std::string s(reinterpret_cast<const char*>(data + pos), len);
        pos += len;
        return s;
    }

    float read_f32() {
        uint32_t bits = read_u32();
        float v;
        static_assert(sizeof(bits) == sizeof(v), "float must be 4 bytes");
        std::memcpy(&v, &bits, sizeof(v));
        return v;
    }
};

} // namespace idoc::byteorder
