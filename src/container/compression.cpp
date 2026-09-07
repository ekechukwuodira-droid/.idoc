#include "idoc/container/compression.hpp"

#include <zstd.h>
#include <stdexcept>

namespace idoc::compression {

std::vector<uint8_t> compress(const std::vector<uint8_t>& data, int level) {
    size_t bound = ZSTD_compressBound(data.size());
    std::vector<uint8_t> out(bound);

    size_t written = ZSTD_compress(out.data(), out.size(),
                                    data.data(), data.size(), level);
    if (ZSTD_isError(written)) {
        throw std::runtime_error(std::string("zstd compress failed: ") +
                                  ZSTD_getErrorName(written));
    }
    out.resize(written);
    return out;
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed) {
    unsigned long long content_size =
        ZSTD_getFrameContentSize(compressed.data(), compressed.size());

    if (content_size == ZSTD_CONTENTSIZE_ERROR) {
        throw std::runtime_error("zstd decompress failed: not a valid zstd frame");
    }
    if (content_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        throw std::runtime_error("zstd decompress failed: frame content size unknown "
                                  "(streaming frames not supported by this wrapper)");
    }

    std::vector<uint8_t> out(static_cast<size_t>(content_size));
    size_t written = ZSTD_decompress(out.data(), out.size(),
                                      compressed.data(), compressed.size());
    if (ZSTD_isError(written)) {
        throw std::runtime_error(std::string("zstd decompress failed: ") +
                                  ZSTD_getErrorName(written));
    }
    out.resize(written);
    return out;
}

} // namespace idoc::compression
