#pragma once
// Numbering Engine — §7. Pure data, no I/O.
//
// ASSUMPTION FLAGGED (Glyph): described only as "Unicode codepoint, or..."
// -- modeled as a plain uint32_t codepoint, which is what "Unicode
// codepoint" unambiguously means; the "or..." trails off into
// `bullet_image` (a ResourceRef), which is already a separate field.
//
// ASSUMPTION FLAGGED (ResourceRef): not defined anywhere in the spec, but
// used here and presumably by other blocks once §11 (Resources) exists.
// Modeled minimally as a single resource_id string -- the same "reference
// by stable ID, don't require the referenced object's full schema"
// treatment already used for ListRef (§6) and style_id (§4). May need to
// move to a shared location once §11 defines what a "resource" actually
// carries (MIME type? size? both?).
//
// ASSUMPTION FLAGGED (LevelOverride): not defined anywhere in the spec.
// The only example given is "this one list starts at 5, not 1", so we've
// modeled exactly that (level + optional start_at override) rather than
// guessing at a fuller per-level override shape. Additive by construction:
// more optional override fields can be appended later without breaking
// this.

#include "idoc/model/paragraph.hpp" // Alignment, Indent, RunProperties

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

constexpr int kNumberingLevelCount = 9; // indices 0-8, per spec

using Glyph = uint32_t; // Unicode codepoint -- see ASSUMPTION FLAGGED note above

struct ResourceRef {
    std::string resource_id;

    bool operator==(const ResourceRef& other) const {
        return resource_id == other.resource_id;
    }
};

enum class NumberFormat : uint8_t {
    kDecimal = 0,
    kUpperRoman = 1,
    kLowerRoman = 2,
    kUpperLetter = 3,
    kLowerLetter = 4,
    kBullet = 5,
    kOrdinal = 6,
    kChineseCounting = 7,
    kCustomGlyph = 8,
    kImage = 9,
};

// Continuous | RestartEachSection | RestartAfterHigherLevel(level_index).
// `level_index` is only meaningful when kind == kRestartAfterHigherLevel.
struct RestartRule {
    enum class Kind : uint8_t {
        kContinuous = 0,
        kRestartEachSection = 1,
        kRestartAfterHigherLevel = 2,
    };

    Kind kind = Kind::kContinuous;
    std::optional<int32_t> level_index;

    bool operator==(const RestartRule& other) const {
        return kind == other.kind && level_index == other.level_index;
    }
};

struct LevelDefinition {
    int32_t level = 0; // 0-8
    NumberFormat format = NumberFormat::kDecimal;
    std::string text_pattern; // e.g. "%1.%2." -- %N references the resolved number at level N
    std::optional<Glyph> bullet_glyph;       // for kBullet
    std::optional<ResourceRef> bullet_image; // for kCustomGlyph/kImage
    Alignment alignment = Alignment::kLeft;
    Indent indent;
    std::optional<RunProperties> number_run_props;
    int32_t start_at = 1;
    RestartRule restart_rule;

    bool operator==(const LevelDefinition& other) const {
        return level == other.level && format == other.format &&
               text_pattern == other.text_pattern && bullet_glyph == other.bullet_glyph &&
               bullet_image == other.bullet_image && alignment == other.alignment &&
               indent == other.indent && number_run_props == other.number_run_props &&
               start_at == other.start_at && restart_rule == other.restart_rule;
    }
};

struct AbstractNum {
    std::string abstract_num_id;
    std::array<LevelDefinition, kNumberingLevelCount> levels;

    bool operator==(const AbstractNum& other) const {
        return abstract_num_id == other.abstract_num_id && levels == other.levels;
    }
};

// See ASSUMPTION FLAGGED (LevelOverride) note above.
struct LevelOverride {
    int32_t level = 0;
    std::optional<int32_t> start_at;

    bool operator==(const LevelOverride& other) const {
        return level == other.level && start_at == other.start_at;
    }
};

struct NumberingInstance {
    std::string instance_id;      // referenced by Paragraph.list_ref (§6)
    std::string abstract_num_id;
    std::optional<std::vector<LevelOverride>> overrides;

    bool operator==(const NumberingInstance& other) const {
        return instance_id == other.instance_id && abstract_num_id == other.abstract_num_id &&
               overrides == other.overrides;
    }
};

// Physical container layout's "Block 4: Numbering Definitions" -- holds
// both AbstractNum[] and NumberingInstance[] together, since a
// NumberingInstance is meaningless without its AbstractNum.
struct NumberingDefinitions {
    std::vector<AbstractNum> abstract_nums;
    std::vector<NumberingInstance> instances;

    bool operator==(const NumberingDefinitions& other) const {
        return abstract_nums == other.abstract_nums && instances == other.instances;
    }
};

} // namespace idoc::model
