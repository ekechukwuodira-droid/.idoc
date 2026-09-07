#include <doctest/doctest.h>
#include "idoc/serde/tables_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::Table make_sample_table() {
    model::Table t;
    t.table_id = "table-1";
    t.props.alignment = model::Alignment::kCenter;
    t.props.layout = model::LayoutAlgorithm::kFixed;
    t.props.repeat_header_rows = true;

    model::Column col1;
    col1.width_type = model::Column::WidthType::kFixed;
    col1.width_twips = 2000;
    model::Column col2;
    col2.width_type = model::Column::WidthType::kPercent;
    col2.width_percent = 50.0f;
    t.columns = {col1, col2};

    model::Cell header_cell1;
    header_cell1.cell_id = "cell-1-1";
    header_cell1.vertical_align = model::CellVerticalAlign::kCenter;
    model::Cell header_cell2;
    header_cell2.cell_id = "cell-1-2";
    header_cell2.vertical_align = model::CellVerticalAlign::kCenter;

    model::Row header_row;
    header_row.row_id = "row-1";
    header_row.is_header_row = true;
    header_row.cells = {header_cell1, header_cell2};

    model::Cell body_cell1;
    body_cell1.cell_id = "cell-2-1";
    body_cell1.col_span = 2;
    body_cell1.row_span = 1;
    body_cell1.merge_role = model::MergeRole::kOrigin;

    model::Row body_row;
    body_row.row_id = "row-2";
    body_row.cant_split = true;
    body_row.cells = {body_cell1};

    t.rows = {header_row, body_row};

    return t;
}

} // namespace

TEST_CASE("tables serde: full round-trip") {
    model::Tables tables;
    tables.tables = {make_sample_table()};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    CHECK(tables2 == tables);
}

TEST_CASE("tables serde: fixed vs percent column widths round-trip distinctly") {
    model::Tables tables;
    tables.tables = {make_sample_table()};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    REQUIRE(tables2.tables[0].columns.size() == 2);
    CHECK(tables2.tables[0].columns[0].width_type == model::Column::WidthType::kFixed);
    CHECK(tables2.tables[0].columns[0].width_twips == 2000);
    CHECK(tables2.tables[0].columns[1].width_type == model::Column::WidthType::kPercent);
    CHECK(tables2.tables[0].columns[1].width_percent == doctest::Approx(50.0f));
}

TEST_CASE("tables serde: merged cell (col_span/row_span/merge_role) round-trips") {
    model::Tables tables;
    tables.tables = {make_sample_table()};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    const auto& cell = tables2.tables[0].rows[1].cells[0];
    CHECK(cell.col_span == 2);
    CHECK(cell.row_span == 1);
    CHECK(cell.merge_role == model::MergeRole::kOrigin);
}

TEST_CASE("tables serde: header row flag and cant_split round-trip") {
    model::Tables tables;
    tables.tables = {make_sample_table()};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    CHECK(tables2.tables[0].rows[0].is_header_row == true);
    CHECK(tables2.tables[0].rows[1].is_header_row == false);
    CHECK(tables2.tables[0].rows[1].cant_split == true);
}

TEST_CASE("tables serde: minimal table (no optional fields) round-trips") {
    model::Table t;
    t.table_id = "table-minimal";
    // props left at defaults, no columns, no rows

    model::Tables tables;
    tables.tables = {t};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    REQUIRE(tables2.tables.size() == 1);
    CHECK(tables2.tables[0].columns.empty());
    CHECK(tables2.tables[0].rows.empty());
    CHECK(tables2.tables[0] == t);
}

TEST_CASE("tables serde: cell-level shading/borders reserved slots and content refs round-trip") {
    model::Cell cell;
    cell.cell_id = "cell-with-reserved";
    cell.shading_raw = std::vector<uint8_t>{0x01};
    cell.borders_raw = std::vector<uint8_t>{0x02, 0x03};
    cell.content = {model::ContentRef{model::ContentType::kParagraph, "para-1"},
                     model::ContentRef{model::ContentType::kTable, "nested-table-1"}};

    model::Row row;
    row.row_id = "row-1";
    row.cells = {cell};

    model::Table t;
    t.table_id = "table-1";
    t.rows = {row};

    model::Tables tables;
    tables.tables = {t};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    const auto& cell2 = tables2.tables[0].rows[0].cells[0];
    CHECK(cell2.shading_raw.value() == std::vector<uint8_t>{0x01});
    CHECK(cell2.borders_raw.value() == std::vector<uint8_t>{0x02, 0x03});
    REQUIRE(cell2.content.size() == 2);
    CHECK(cell2.content[0].type == model::ContentType::kParagraph);
    CHECK(cell2.content[0].content_id == "para-1");
    CHECK(cell2.content[1].type == model::ContentType::kTable);
    CHECK(cell2.content[1].content_id == "nested-table-1");
}

TEST_CASE("tables serde: multiple tables preserve order") {
    model::Table t1; t1.table_id = "table-1";
    model::Table t2; t2.table_id = "table-2";

    model::Tables tables;
    tables.tables = {t1, t2};

    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);

    REQUIRE(tables2.tables.size() == 2);
    CHECK(tables2.tables[0].table_id == "table-1");
    CHECK(tables2.tables[1].table_id == "table-2");
}

TEST_CASE("tables serde: empty Tables round-trips") {
    model::Tables tables;
    auto payload = serde::serialize_tables(tables);
    auto tables2 = serde::deserialize_tables(payload);
    CHECK(tables2.tables.empty());
}

TEST_CASE("tables serde: unknown field within a Table is skipped") {
    model::Tables tables;
    tables.tables = {make_sample_table()};
    auto payload = serde::serialize_tables(tables);

    auto outer_records = tlv::parse_records(payload);
    REQUIRE(outer_records.size() == 1);
    std::vector<uint8_t> patched_inner = outer_records[0].payload;
    tlv::write_record(patched_inner, 999, 1, {0x01});

    std::vector<uint8_t> patched_payload;
    tlv::write_record(patched_payload, outer_records[0].header.type_id,
                       outer_records[0].header.schema_version, patched_inner);

    auto tables2 = serde::deserialize_tables(patched_payload);
    REQUIRE(tables2.tables.size() == 1);
    CHECK(tables2.tables[0].table_id == "table-1");
}

TEST_CASE("tables serde: unknown top-level record type is skipped, not fatal") {
    model::Tables tables;
    auto payload = serde::serialize_tables(tables);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto tables2 = serde::deserialize_tables(payload);
    CHECK(tables2.tables.empty());
}
