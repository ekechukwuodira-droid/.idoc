#include "idoc/serde/numbering_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

#include <stdexcept>

namespace idoc::serde {

namespace record_type {
// Top-level repeated record types within the NumberingDefinitions block.
constexpr uint32_t kAbstractNum = 1;
constexpr uint32_t kNumberingInstance = 2;
} // namespace record_type

namespace level_field {
constexpr uint32_t kLevel = 1;
constexpr uint32_t kFormat = 2;
constexpr uint32_t kTextPattern = 3;
constexpr uint32_t kBulletGlyph = 4;
constexpr uint32_t kBulletImage = 5;
constexpr uint32_t kAlignment = 6;
constexpr uint32_t kIndent = 7;
constexpr uint32_t kNumberRunProps = 8;
constexpr uint32_t kStartAt = 9;
constexpr uint32_t kRestartRule = 10;
} // namespace level_field

namespace abstract_num_field {
constexpr uint32_t kAbstractNumId = 1;
constexpr uint32_t kLevel = 2; // repeated, exactly kNumberingLevelCount times
} // namespace abstract_num_field

namespace instance_field {
constexpr uint32_t kInstanceId = 1;
constexpr uint32_t kAbstractNumId = 2;
constexpr uint32_t kOverrides = 3; // optional, single record wraps the whole list
} // namespace instance_field

namespace {

std::vector<uint8_t> encode_level_definition(const model::LevelDefinition& lvl) {
    std::vector<uint8_t> out;

    tlv::write_record(out, level_field::kLevel, kNumberingSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, static_cast<uint32_t>(lvl.level)); return p; }());
    tlv::write_record(out, level_field::kFormat, kNumberingSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(lvl.format)});
    tlv::write_record(out, level_field::kTextPattern, kNumberingSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_string(p, lvl.text_pattern); return p; }());

    if (lvl.bullet_glyph.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_u32(p, *lvl.bullet_glyph);
        tlv::write_record(out, level_field::kBulletGlyph, kNumberingSchemaVersion, p);
    }
    if (lvl.bullet_image.has_value()) {
        std::vector<uint8_t> p;
        byteorder::write_string(p, lvl.bullet_image->resource_id);
        tlv::write_record(out, level_field::kBulletImage, kNumberingSchemaVersion, p);
    }

    tlv::write_record(out, level_field::kAlignment, kNumberingSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(lvl.alignment)});
    tlv::write_record(out, level_field::kIndent, kNumberingSchemaVersion, common::encode_indent(lvl.indent));

    if (lvl.number_run_props.has_value()) {
        tlv::write_record(out, level_field::kNumberRunProps, kNumberingSchemaVersion,
                           common::encode_run_properties(*lvl.number_run_props, kNumberingSchemaVersion));
    }

    tlv::write_record(out, level_field::kStartAt, kNumberingSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, static_cast<uint32_t>(lvl.start_at)); return p; }());
    tlv::write_record(out, level_field::kRestartRule, kNumberingSchemaVersion, common::encode_restart_rule(lvl.restart_rule));

    return out;
}

model::LevelDefinition decode_level_definition(const std::vector<uint8_t>& payload) {
    model::LevelDefinition lvl;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case level_field::kLevel:
                lvl.level = static_cast<int32_t>(r.read_u32());
                break;
            case level_field::kFormat:
                lvl.format = static_cast<model::NumberFormat>(r.read_u8());
                break;
            case level_field::kTextPattern:
                lvl.text_pattern = r.read_string();
                break;
            case level_field::kBulletGlyph:
                lvl.bullet_glyph = r.read_u32();
                break;
            case level_field::kBulletImage: {
                model::ResourceRef ref;
                ref.resource_id = r.read_string();
                lvl.bullet_image = ref;
                break;
            }
            case level_field::kAlignment:
                lvl.alignment = static_cast<model::Alignment>(r.read_u8());
                break;
            case level_field::kIndent:
                lvl.indent = common::decode_indent(rec.payload);
                break;
            case level_field::kNumberRunProps:
                lvl.number_run_props = common::decode_run_properties(rec.payload);
                break;
            case level_field::kStartAt:
                lvl.start_at = static_cast<int32_t>(r.read_u32());
                break;
            case level_field::kRestartRule:
                lvl.restart_rule = common::decode_restart_rule(rec.payload);
                break;
            default:
                break; // unknown field: skip
        }
    }
    return lvl;
}

std::vector<uint8_t> encode_abstract_num(const model::AbstractNum& an) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, an.abstract_num_id);
        tlv::write_record(out, abstract_num_field::kAbstractNumId, kNumberingSchemaVersion, p);
    }
    // Array position is authoritative for a fixed 9-slot array -- encode
    // the loop index as the level number rather than trusting each
    // element's own `.level` field, which could be stale or (for a
    // default-constructed element) collide with another level's value.
    for (size_t i = 0; i < an.levels.size(); ++i) {
        model::LevelDefinition lvl = an.levels[i];
        lvl.level = static_cast<int32_t>(i);
        tlv::write_record(out, abstract_num_field::kLevel, kNumberingSchemaVersion,
                           encode_level_definition(lvl));
    }

    return out;
}

model::AbstractNum decode_abstract_num(const std::vector<uint8_t>& payload) {
    model::AbstractNum an;
    auto records = tlv::parse_records(payload);

    // Levels are always written in order 0..8 (see encode_abstract_num),
    // so position is derived from encounter order, not from the decoded
    // `level` field -- this is what keeps default-valued levels from
    // colliding with a real level 0.
    size_t next_index = 0;
    for (const auto& rec : records) {
        if (rec.header.type_id == abstract_num_field::kAbstractNumId) {
            byteorder::Reader r(rec.payload);
            an.abstract_num_id = r.read_string();
        } else if (rec.header.type_id == abstract_num_field::kLevel) {
            if (next_index < an.levels.size()) {
                an.levels[next_index] = decode_level_definition(rec.payload);
                ++next_index;
            }
            // more than 9 level records: extras ignored defensively
        }
        // unknown top-level field: skip
    }
    return an;
}

std::vector<uint8_t> encode_overrides(const std::vector<model::LevelOverride>& overrides) {
    std::vector<uint8_t> out;
    byteorder::write_u32(out, static_cast<uint32_t>(overrides.size()));
    for (const auto& ov : overrides) {
        byteorder::write_u32(out, static_cast<uint32_t>(ov.level));
        byteorder::write_u8(out, ov.start_at.has_value() ? 1 : 0);
        if (ov.start_at.has_value()) byteorder::write_u32(out, static_cast<uint32_t>(*ov.start_at));
    }
    return out;
}

std::vector<model::LevelOverride> decode_overrides(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    std::vector<model::LevelOverride> overrides;
    uint32_t count = r.read_u32();
    overrides.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        model::LevelOverride ov;
        ov.level = static_cast<int32_t>(r.read_u32());
        if (r.read_u8() != 0) ov.start_at = static_cast<int32_t>(r.read_u32());
        overrides.push_back(ov);
    }
    return overrides;
}

std::vector<uint8_t> encode_numbering_instance(const model::NumberingInstance& inst) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, inst.instance_id);
        tlv::write_record(out, instance_field::kInstanceId, kNumberingSchemaVersion, p);
    }
    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, inst.abstract_num_id);
        tlv::write_record(out, instance_field::kAbstractNumId, kNumberingSchemaVersion, p);
    }
    if (inst.overrides.has_value()) {
        tlv::write_record(out, instance_field::kOverrides, kNumberingSchemaVersion,
                           encode_overrides(*inst.overrides));
    }

    return out;
}

model::NumberingInstance decode_numbering_instance(const std::vector<uint8_t>& payload) {
    model::NumberingInstance inst;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case instance_field::kInstanceId:
                inst.instance_id = r.read_string();
                break;
            case instance_field::kAbstractNumId:
                inst.abstract_num_id = r.read_string();
                break;
            case instance_field::kOverrides:
                inst.overrides = decode_overrides(rec.payload);
                break;
            default:
                break; // unknown field: skip
        }
    }
    return inst;
}

} // namespace

std::vector<uint8_t> serialize_numbering_definitions(const model::NumberingDefinitions& nd) {
    std::vector<uint8_t> out;
    for (const auto& an : nd.abstract_nums) {
        tlv::write_record(out, record_type::kAbstractNum, kNumberingSchemaVersion, encode_abstract_num(an));
    }
    for (const auto& inst : nd.instances) {
        tlv::write_record(out, record_type::kNumberingInstance, kNumberingSchemaVersion,
                           encode_numbering_instance(inst));
    }
    return out;
}

model::NumberingDefinitions deserialize_numbering_definitions(const std::vector<uint8_t>& payload) {
    model::NumberingDefinitions nd;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        if (rec.header.type_id == record_type::kAbstractNum) {
            nd.abstract_nums.push_back(decode_abstract_num(rec.payload));
        } else if (rec.header.type_id == record_type::kNumberingInstance) {
            nd.instances.push_back(decode_numbering_instance(rec.payload));
        }
        // unknown top-level record type: skip
    }
    return nd;
}

} // namespace idoc::serde
