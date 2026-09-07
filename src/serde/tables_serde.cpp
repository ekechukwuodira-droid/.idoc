#include "idoc/serde/tables_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"

namespace idoc::serde {

namespace record_type {
constexpr uint32_t kTable = 1; // only repeated record type at the block's top level
} // namespace record_type

namespace props_field {
constexpr uint32_t kAlignment = 1;
constexpr uint32_t kBordersRaw = 2;         // reserved, see model/tables.hpp
constexpr uint32_t kDefaultShadingRaw = 3;  // reserved
constexpr uint32_t kLayout = 4;
constexpr uint32_t kRepeatHeaderRows = 5;
} // namespace props_field

namespace cell_field {
constexpr uint32_t kCellId = 1;
constexpr uint32_t kColSpan = 2;
constexpr uint32_t kRowSpan = 3;
constexpr uint32_t kMergeRole = 4;
constexpr uint32_t kShadingRaw = 5;   // reserved
constexpr uint32_t kBordersRaw = 6;   // reserved
constexpr uint32_t kVerticalAlign = 7;
constexpr uint32_t kContentRaw = 8;   // reserved, see model/tables.hpp
} // namespace cell_field

namespace row_field {
constexpr uint32_t kRowId = 1;
constexpr uint32_t kIsHeaderRow = 2;
constexpr uint32_t kCantSplit = 3;
constexpr uint32_t kCell = 4; // repeated
} // namespace row_field

namespace table_field {
constexpr uint32_t kTableId = 1;
constexpr uint32_t kProps = 2;
constexpr uint32_t kColumn = 3; // repeated
constexpr uint32_t kRow = 4;    // repeated
} // namespace table_field

namespace {

// --- Column: simple fixed-shape payload, no nested TLV needed ---

std::vector<uint8_t> encode_column(const model::Column& c) {
    std::vector<uint8_t> out;
    byteorder::write_u8(out, static_cast<uint8_t>(c.width_type));
    byteorder::write_u32(out, c.width_twips);
    byteorder::write_f32(out, c.width_percent);
    return out;
}

model::Column decode_column(const std::vector<uint8_t>& payload) {
    byteorder::Reader r(payload);
    model::Column c;
    c.width_type = static_cast<model::Column::WidthType>(r.read_u8());
    c.width_twips = r.read_u32();
    c.width_percent = r.read_f32();
    return c;
}

// --- TableProperties: nested TLV field list ---

std::vector<uint8_t> encode_properties(const model::TableProperties& props) {
    std::vector<uint8_t> out;

    tlv::write_record(out, props_field::kAlignment, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(props.alignment)});
    if (props.borders_raw.has_value()) {
        tlv::write_record(out, props_field::kBordersRaw, kTablesSchemaVersion, *props.borders_raw);
    }
    if (props.default_shading_raw.has_value()) {
        tlv::write_record(out, props_field::kDefaultShadingRaw, kTablesSchemaVersion, *props.default_shading_raw);
    }
    tlv::write_record(out, props_field::kLayout, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(props.layout)});
    tlv::write_record(out, props_field::kRepeatHeaderRows, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(props.repeat_header_rows ? 1 : 0)});

    return out;
}

model::TableProperties decode_properties(const std::vector<uint8_t>& payload) {
    model::TableProperties props;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case props_field::kAlignment:
                props.alignment = static_cast<model::Alignment>(r.read_u8());
                break;
            case props_field::kBordersRaw:
                props.borders_raw = rec.payload;
                break;
            case props_field::kDefaultShadingRaw:
                props.default_shading_raw = rec.payload;
                break;
            case props_field::kLayout:
                props.layout = static_cast<model::LayoutAlgorithm>(r.read_u8());
                break;
            case props_field::kRepeatHeaderRows:
                props.repeat_header_rows = r.read_u8() != 0;
                break;
            default:
                break; // unknown field: skip
        }
    }
    return props;
}

// --- Cell: nested TLV field list ---

std::vector<uint8_t> encode_cell(const model::Cell& cell) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, cell.cell_id);
        tlv::write_record(out, cell_field::kCellId, kTablesSchemaVersion, p);
    }
    tlv::write_record(out, cell_field::kColSpan, kTablesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, static_cast<uint32_t>(cell.col_span)); return p; }());
    tlv::write_record(out, cell_field::kRowSpan, kTablesSchemaVersion,
                       [&] { std::vector<uint8_t> p; byteorder::write_u32(p, static_cast<uint32_t>(cell.row_span)); return p; }());
    tlv::write_record(out, cell_field::kMergeRole, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(cell.merge_role)});

    if (cell.shading_raw.has_value()) {
        tlv::write_record(out, cell_field::kShadingRaw, kTablesSchemaVersion, *cell.shading_raw);
    }
    if (cell.borders_raw.has_value()) {
        tlv::write_record(out, cell_field::kBordersRaw, kTablesSchemaVersion, *cell.borders_raw);
    }

    tlv::write_record(out, cell_field::kVerticalAlign, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(cell.vertical_align)});

    if (cell.content_raw.has_value()) {
        tlv::write_record(out, cell_field::kContentRaw, kTablesSchemaVersion, *cell.content_raw);
    }

    return out;
}

model::Cell decode_cell(const std::vector<uint8_t>& payload) {
    model::Cell cell;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case cell_field::kCellId:
                cell.cell_id = r.read_string();
                break;
            case cell_field::kColSpan:
                cell.col_span = static_cast<int32_t>(r.read_u32());
                break;
            case cell_field::kRowSpan:
                cell.row_span = static_cast<int32_t>(r.read_u32());
                break;
            case cell_field::kMergeRole:
                cell.merge_role = static_cast<model::MergeRole>(r.read_u8());
                break;
            case cell_field::kShadingRaw:
                cell.shading_raw = rec.payload;
                break;
            case cell_field::kBordersRaw:
                cell.borders_raw = rec.payload;
                break;
            case cell_field::kVerticalAlign:
                cell.vertical_align = static_cast<model::CellVerticalAlign>(r.read_u8());
                break;
            case cell_field::kContentRaw:
                cell.content_raw = rec.payload;
                break;
            default:
                break; // unknown field: skip
        }
    }
    return cell;
}

// --- Row: nested TLV field list, with repeated Cell records ---

std::vector<uint8_t> encode_row(const model::Row& row) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, row.row_id);
        tlv::write_record(out, row_field::kRowId, kTablesSchemaVersion, p);
    }
    tlv::write_record(out, row_field::kIsHeaderRow, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(row.is_header_row ? 1 : 0)});
    tlv::write_record(out, row_field::kCantSplit, kTablesSchemaVersion,
                       std::vector<uint8_t>{static_cast<uint8_t>(row.cant_split ? 1 : 0)});

    for (const auto& cell : row.cells) {
        tlv::write_record(out, row_field::kCell, kTablesSchemaVersion, encode_cell(cell));
    }

    return out;
}

model::Row decode_row(const std::vector<uint8_t>& payload) {
    model::Row row;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        byteorder::Reader r(rec.payload);
        switch (rec.header.type_id) {
            case row_field::kRowId:
                row.row_id = r.read_string();
                break;
            case row_field::kIsHeaderRow:
                row.is_header_row = r.read_u8() != 0;
                break;
            case row_field::kCantSplit:
                row.cant_split = r.read_u8() != 0;
                break;
            case row_field::kCell:
                row.cells.push_back(decode_cell(rec.payload));
                break;
            default:
                break; // unknown field: skip
        }
    }
    return row;
}

// --- Table: nested TLV field list, with repeated Column and Row records ---

std::vector<uint8_t> encode_table(const model::Table& table) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> p;
        byteorder::write_string(p, table.table_id);
        tlv::write_record(out, table_field::kTableId, kTablesSchemaVersion, p);
    }
    tlv::write_record(out, table_field::kProps, kTablesSchemaVersion, encode_properties(table.props));

    for (const auto& col : table.columns) {
        tlv::write_record(out, table_field::kColumn, kTablesSchemaVersion, encode_column(col));
    }
    for (const auto& row : table.rows) {
        tlv::write_record(out, table_field::kRow, kTablesSchemaVersion, encode_row(row));
    }

    return out;
}

model::Table decode_table(const std::vector<uint8_t>& payload) {
    model::Table table;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        switch (rec.header.type_id) {
            case table_field::kTableId: {
                byteorder::Reader r(rec.payload);
                table.table_id = r.read_string();
                break;
            }
            case table_field::kProps:
                table.props = decode_properties(rec.payload);
                break;
            case table_field::kColumn:
                table.columns.push_back(decode_column(rec.payload));
                break;
            case table_field::kRow:
                table.rows.push_back(decode_row(rec.payload));
                break;
            default:
                break; // unknown field: skip
        }
    }
    return table;
}

} // namespace

std::vector<uint8_t> serialize_tables(const model::Tables& tables) {
    std::vector<uint8_t> out;
    for (const auto& t : tables.tables) {
        tlv::write_record(out, record_type::kTable, kTablesSchemaVersion, encode_table(t));
    }
    return out;
}

model::Tables deserialize_tables(const std::vector<uint8_t>& payload) {
    model::Tables tables;
    auto records = tlv::parse_records(payload);
    for (const auto& rec : records) {
        if (rec.header.type_id != record_type::kTable) continue; // unknown top-level record: skip
        tables.tables.push_back(decode_table(rec.payload));
    }
    return tables;
}

} // namespace idoc::serde
