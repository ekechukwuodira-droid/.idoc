#pragma once
// Section & PageSetup — §5. Pure data, no I/O.
//
// RESOLVED: `Section.content` and `HeaderFooterContent.content` are
// `Block[]` (Paragraph | Table) in the spec. This used to be a reserved
// raw byte slot pending a decision on the reference format; that
// decision is now made in model/content_ref.hpp -- see that file for the
// full reasoning. Both fields are now `std::vector<ContentRef>`.
//
// ASSUMPTION FLAGGED: the spec names `page_number_restart`'s type
// `NumberingRestart` but never defines it. §7/§8 define a `RestartRule`
// enum (Continuous | RestartEachSection | RestartAfterHigherLevel) for
// numbering *levels*, but that doesn't fit page-number semantics --
// "restart lettering after a higher list level changes" has no
// equivalent for a page counter. What real word processors expose here
// is simpler: a bool "restart numbering in this section" plus an
// optional explicit start value. We've modeled it that way
// (NumberingRestart::restart + start_at) rather than reusing RestartRule
// verbatim, since reusing it would mean either a level_index field that
// means nothing for pages, or a variant that only sometimes applies.

#include "idoc/model/content_ref.hpp" // ContentRef

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

enum class Orientation : uint8_t {
    kPortrait = 0,
    kLandscape = 1,
};

struct Size2D {
    uint32_t width = 0;   // twips
    uint32_t height = 0;  // twips

    bool operator==(const Size2D& other) const {
        return width == other.width && height == other.height;
    }
};

struct Margins {
    int32_t top = 0;      // twips; signed -- some layouts use negative margins deliberately
    int32_t bottom = 0;
    int32_t left = 0;
    int32_t right = 0;
    uint32_t gutter = 0;  // twips; binding gutter, always additive/non-negative

    bool operator==(const Margins& other) const {
        return top == other.top && bottom == other.bottom &&
               left == other.left && right == other.right && gutter == other.gutter;
    }
};

struct ColumnLayout {
    uint32_t count = 1;
    uint32_t spacing = 0;   // twips, between columns
    bool equal_width = true;
    bool rule_line = false; // vertical rule between columns

    bool operator==(const ColumnLayout& other) const {
        return count == other.count && spacing == other.spacing &&
               equal_width == other.equal_width && rule_line == other.rule_line;
    }
};

struct PageSetup {
    Size2D page_size;
    Margins margins;
    Orientation orientation = Orientation::kPortrait;
    ColumnLayout columns;
    bool different_first_page = false;
    bool different_odd_even = false;
    std::optional<std::string> paper_source; // print tray identifier; spec doesn't define PaperSource's shape

    bool operator==(const PageSetup& other) const {
        return page_size == other.page_size && margins == other.margins &&
               orientation == other.orientation && columns == other.columns &&
               different_first_page == other.different_first_page &&
               different_odd_even == other.different_odd_even &&
               paper_source == other.paper_source;
    }
};

// See ASSUMPTION FLAGGED note above.
struct NumberingRestart {
    bool restart = false;
    std::optional<uint32_t> start_at;

    bool operator==(const NumberingRestart& other) const {
        return restart == other.restart && start_at == other.start_at;
    }
};

struct HeaderFooterContent {
    // Block[] (Paragraph | Table) -- see model/content_ref.hpp.
    std::vector<ContentRef> content;

    bool operator==(const HeaderFooterContent& other) const {
        return content == other.content;
    }
};

struct HeaderFooterSet {
    std::optional<HeaderFooterContent> default_;
    std::optional<HeaderFooterContent> first;
    std::optional<HeaderFooterContent> even;

    bool operator==(const HeaderFooterSet& other) const {
        return default_ == other.default_ && first == other.first && even == other.even;
    }
};

struct Section {
    std::string section_id;
    PageSetup page_setup;
    HeaderFooterSet headers;
    HeaderFooterSet footers;
    std::optional<NumberingRestart> page_number_restart;
    // Block[] (Paragraph | Table) -- see model/content_ref.hpp.
    std::vector<ContentRef> content;

    bool operator==(const Section& other) const {
        return section_id == other.section_id && page_setup == other.page_setup &&
               headers == other.headers && footers == other.footers &&
               page_number_restart == other.page_number_restart &&
               content == other.content;
    }
};

struct Sections {
    std::vector<Section> sections;

    bool operator==(const Sections& other) const {
        return sections == other.sections;
    }
};

} // namespace idoc::model
