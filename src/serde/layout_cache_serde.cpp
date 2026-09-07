#include "idoc/serde/layout_cache_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"

namespace idoc::serde {

namespace top_field {
constexpr uint32_t kGeneration = 1;
constexpr uint32_t kPage = 2;         // repeated
constexpr uint32_t kFieldResult = 3;  // repeated
} // namespace top_field

namespace page_field {
constexpr uint32_t kPageNumber = 1;
constexpr uint32_t kSectionId = 2;
constexpr uint32_t kContentBlockRefsRaw = 3; // reserved, see model/layout_cache.hpp
constexpr uint32_t kFieldId = 4;             // repeated
} // namespace page_field

namespace {

std::vector<uint8_t> encode_page(const model::PageGeometry& page) {
    std::vector<uint8_t> out;

    tlv::write_record(out, page_field::kPageNumber, kLayoutCacheSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, page.page_number); return p; }());
    tlv::write_record(out, page_field::kSectionId, kLayoutCacheSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, page.section_id); return p; }());

    if (page.content_block_refs_raw.has_value()) {
        tlv::write_record(out, page_field::kContentBlockRefsRaw, kLayoutCacheSchemaVersion,
                           *page.content_block_refs_raw);
    }

    for (const auto& field_id : page.field_ids) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, field_id);
        tlv::write_record(out, page_field::kFieldId, kLayoutCacheSchemaVersion, p);
    }

    return out;
}

model::PageGeometry decode_page(const std::vector<uint8_t>& payload) {
    model::PageGeometry page;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case page_field::kPageNumber:
                page.page_number = r.read_u32();
                break;
            case page_field::kSectionId:
                page.section_id = r.read_string();
                break;
            case page_field::kContentBlockRefsRaw:
                page.content_block_refs_raw = rec.payload;
                break;
            case page_field::kFieldId:
                page.field_ids.push_back(r.read_string());
                break;
            default:
                break; // unknown field: skip
        }
    }
    return page;
}

std::vector<uint8_t> encode_field_result(const std::string& field_id, const std::string& cached_result) {
    std::vector<uint8_t> p;
    byteorder::write_string(p, field_id);
    byteorder::write_string(p, cached_result);
    return p;
}

} // namespace

std::vector<uint8_t> serialize_layout_cache(const model::LayoutCache& lc) {
    std::vector<uint8_t> out;

    tlv::write_record(out, top_field::kGeneration, kLayoutCacheSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, lc.generation); return p; }());

    for (const auto& page : lc.pages) {
        tlv::write_record(out, top_field::kPage, kLayoutCacheSchemaVersion, encode_page(page));
    }

    for (const auto& [field_id, cached_result] : lc.field_results) {
        tlv::write_record(out, top_field::kFieldResult, kLayoutCacheSchemaVersion,
                           encode_field_result(field_id, cached_result));
    }

    return out;
}

model::LayoutCache deserialize_layout_cache(const std::vector<uint8_t>& payload) {
    model::LayoutCache lc;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case top_field::kGeneration:
                lc.generation = r.read_u32();
                break;
            case top_field::kPage:
                lc.pages.push_back(decode_page(rec.payload));
                break;
            case top_field::kFieldResult: {
                std::string field_id = r.read_string();
                std::string cached_result = r.read_string();
                lc.field_results[field_id] = cached_result;
                break;
            }
            default:
                break; // unknown field: skip
        }
    }
    return lc;
}

} // namespace idoc::serde
