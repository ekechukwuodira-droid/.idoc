#pragma once
// Tables — §9. Pure data, no I/O.
//
// DEFERRED: `TableProperties.borders`/`default_shading`, `Cell.shading`/
// `borders`, and `Cell.content` (`Block[]` -- paragraphs or nested
// tables) are all reserved raw byte slots, for the same reasons already
// established: BorderSet/Shading have no defined wire shape anywhere in
// the spec (same as ParagraphProperties' equivalents), and `Block[]` hits
// the same Section.content architecture question flagged in
// serde/paragraph_serde.hpp -- a table cell's content needs the same
// {content_type, content_id} reference resolution once that's decided.
//
// Note `TableProperties.borders` has no `?` in the spec (implying
// required), unlike `default_shading?` and the Cell-level equivalents
// which are explicitly optional. Since we can't require a value we have
// no shape for, it's modeled as optional here too, with a note that it
// should become non-optional once BorderSet exists and the spec's intent
// can actually be honored.
//
// ASSUMPTION FLAGGED: Column width is described only as "width per
// column (twips), can be % or fixed" -- read as a tagged value (fixed
// twips OR a percentage of table width), not two separate fields. Percent
// is modeled as a float (e.g. 33.33 means 33.33%) rather than a
// fixed-point encoding, since the spec doesn't specify decimal precision
// either way.
//
// `CellVerticalAlign` (Top | Center | Bottom) is a distinct type from
// `TextVerticalAlign` (Baseline | Superscript | Subscript, §6) despite
// the spec calling both "VerticalAlign" -- same collision-avoidance
// treatment as that one.

#include "idoc/model/paragraph.hpp" // Alignment

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

enum class LayoutAlgorithm : uint8_t {
    kFixed = 0,
    kAutoFit = 1,
};

struct TableProperties {
    Alignment alignment = Alignment::kLeft;
    std::optional<std::vector<uint8_t>> borders_raw;        // reserved, see DEFERRED note
    std::optional<std::vector<uint8_t>> default_shading_raw; // reserved, see DEFERRED note
    LayoutAlgorithm layout = LayoutAlgorithm::kFixed;
    bool repeat_header_rows = false;

    bool operator==(const TableProperties& other) const {
        return alignment == other.alignment && borders_raw == other.borders_raw &&
               default_shading_raw == other.default_shading_raw && layout == other.layout &&
               repeat_header_rows == other.repeat_header_rows;
    }
};

// See ASSUMPTION FLAGGED note above.
struct Column {
    enum class WidthType : uint8_t {
        kFixed = 0,   // width_twips is authoritative
        kPercent = 1, // width_percent is authoritative
    };

    WidthType width_type = WidthType::kFixed;
    uint32_t width_twips = 0;
    float width_percent = 0.0f;

    bool operator==(const Column& other) const {
        return width_type == other.width_type && width_twips == other.width_twips &&
               width_percent == other.width_percent;
    }
};

enum class MergeRole : uint8_t {
    kNone = 0,
    kOrigin = 1,
    kContinuation = 2,
};

enum class CellVerticalAlign : uint8_t {
    kTop = 0,
    kCenter = 1,
    kBottom = 2,
};

struct Cell {
    std::string cell_id;
    int32_t col_span = 1;
    int32_t row_span = 1;
    MergeRole merge_role = MergeRole::kNone;
    std::optional<std::vector<uint8_t>> shading_raw; // reserved, see DEFERRED note
    std::optional<std::vector<uint8_t>> borders_raw; // reserved, see DEFERRED note
    CellVerticalAlign vertical_align = CellVerticalAlign::kTop;
    // Reserved for Block[] (Paragraph | Table) -- see DEFERRED note above.
    std::optional<std::vector<uint8_t>> content_raw;

    bool operator==(const Cell& other) const {
        return cell_id == other.cell_id && col_span == other.col_span &&
               row_span == other.row_span && merge_role == other.merge_role &&
               shading_raw == other.shading_raw && borders_raw == other.borders_raw &&
               vertical_align == other.vertical_align && content_raw == other.content_raw;
    }
};

struct Row {
    std::string row_id;
    bool is_header_row = false;
    bool cant_split = false;
    std::vector<Cell> cells;

    bool operator==(const Row& other) const {
        return row_id == other.row_id && is_header_row == other.is_header_row &&
               cant_split == other.cant_split && cells == other.cells;
    }
};

struct Table {
    std::string table_id;
    TableProperties props;
    std::vector<Column> columns;
    std::vector<Row> rows;

    bool operator==(const Table& other) const {
        return table_id == other.table_id && props == other.props &&
               columns == other.columns && rows == other.rows;
    }
};

// Physical container layout's "Block 6: Tables".
struct Tables {
    std::vector<Table> tables;

    bool operator==(const Tables& other) const {
        return tables == other.tables;
    }
};

} // namespace idoc::model
