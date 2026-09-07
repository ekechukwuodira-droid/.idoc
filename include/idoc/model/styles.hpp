#pragma once
// StyleDefinition — §4.2. Pure data, no I/O.
//
// DEFERRED: `paragraph_props` and `run_props` need the full
// ParagraphProperties / RunProperties model from §6, which is a later
// stage. They're represented here as reserved-but-empty optional raw byte
// slots so the TLV field IDs are locked in now -- once §6's types exist,
// populating these becomes a serde-only change (encode/decode the real
// struct into the slot), with no changes to this header, the block
// layout, or anything already shipped.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

enum class StyleType : uint8_t {
    kParagraph = 0,
    kCharacter = 1,
    kTable = 2,
    kNumbering = 3,
    kLinked = 4,
};

struct StyleDefinition {
    std::string style_id;                 // stable, referenced by Paragraph/Run.style_id
    std::string display_name;
    StyleType type = StyleType::kParagraph;
    std::optional<std::string> based_on;  // parent style_id -- inheritance chain
    std::optional<std::string> next_style;
    bool is_default = false;
    bool quick_style = false;

    // Reserved for §6's ParagraphProperties/RunProperties -- always empty
    // (std::nullopt) until that stage exists. See DEFERRED note above.
    std::optional<std::vector<uint8_t>> paragraph_props_raw;
    std::optional<std::vector<uint8_t>> run_props_raw;

    bool operator==(const StyleDefinition& other) const {
        return style_id == other.style_id &&
               display_name == other.display_name &&
               type == other.type &&
               based_on == other.based_on &&
               next_style == other.next_style &&
               is_default == other.is_default &&
               quick_style == other.quick_style &&
               paragraph_props_raw == other.paragraph_props_raw &&
               run_props_raw == other.run_props_raw;
    }
};

struct Styles {
    std::vector<StyleDefinition> definitions;

    bool operator==(const Styles& other) const {
        return definitions == other.definitions;
    }
};

} // namespace idoc::model
