#pragma once
// StyleDefinition — §4.2. Pure data, no I/O.
//
// RESOLVED: `paragraph_props` and `run_props` now hold real
// `ParagraphProperties`/`RunProperties` (§6), populated once the style
// resolver (resolve/style_resolver.hpp) needed something real to walk
// the based_on chain against. Both are optional at the StyleDefinition
// level for the same reason every ParagraphProperties/RunProperties
// field is itself optional: a style in the middle of a based_on chain
// may set no paragraph-level formatting at all and exist purely to
// carry run-level formatting (or vice versa) -- absence here means
// "this style contributes nothing at this level, keep walking the
// chain," not "this style resets everything to empty."

#include "idoc/model/paragraph.hpp" // ParagraphProperties, RunProperties

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

    std::optional<ParagraphProperties> paragraph_props;
    std::optional<RunProperties> run_props;

    bool operator==(const StyleDefinition& other) const {
        return style_id == other.style_id &&
               display_name == other.display_name &&
               type == other.type &&
               based_on == other.based_on &&
               next_style == other.next_style &&
               is_default == other.is_default &&
               quick_style == other.quick_style &&
               paragraph_props == other.paragraph_props &&
               run_props == other.run_props;
    }
};

struct Styles {
    std::vector<StyleDefinition> definitions;

    bool operator==(const Styles& other) const {
        return definitions == other.definitions;
    }
};

} // namespace idoc::model

