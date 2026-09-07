#include <doctest/doctest.h>
#include "idoc/container/block_types.hpp"
#include "idoc/container/container_reader.hpp"
#include "idoc/container/container_writer.hpp"
#include "idoc/serde/metadata_serde.hpp"
#include "idoc/serde/numbering_serde.hpp"
#include "idoc/serde/paragraph_serde.hpp"
#include "idoc/serde/sections_serde.hpp"
#include "idoc/serde/styles_serde.hpp"
#include "idoc/serde/theme_serde.hpp"
#include "idoc/serde/fields_serde.hpp"
#include "idoc/serde/tables_serde.hpp"
#include "idoc/serde/annotations_serde.hpp"
#include "idoc/serde/resources_serde.hpp"
#include "idoc/serde/preserved_unknown_serde.hpp"
#include "idoc/serde/layout_cache_serde.hpp"
#include "idoc/model/numbering.hpp"
#include "idoc/model/paragraph.hpp"
#include "idoc/model/sections.hpp"
#include "idoc/model/styles.hpp"
#include "idoc/model/theme.hpp"
#include "idoc/model/fields.hpp"
#include "idoc/model/tables.hpp"
#include "idoc/model/annotations.hpp"
#include "idoc/model/resources.hpp"
#include "idoc/model/preserved_unknown.hpp"
#include "idoc/model/layout_cache.hpp"

#include <stdexcept>

using namespace idoc;

namespace {

model::Metadata make_sample_metadata() {
    model::Metadata m;
    m.document_id = "b3f1e2a0-roundtrip";
    m.title = "Sample Document";
    m.author = "Test Author";
    m.language = "en-US";
    m.created_at = "2026-08-30T12:00:00Z";
    m.modified_at = "2026-08-30T12:00:00Z";
    m.application_version = "idoc-engine-tests/0.1";
    m.revision_number = 1;
    return m;
}

} // namespace

TEST_CASE("container: full write -> read round-trip with compression") {
    model::Metadata m = make_sample_metadata();

    ContainerWriter writer;
    writer.set_document_id(m.document_id);
    writer.set_created_at(m.created_at);
    writer.set_created_by("idoc-engine-tests");
    writer.set_last_modified_by("idoc-engine-tests");

    auto payload = serde::serialize_metadata(m);
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      payload, /*compress=*/true);

    std::vector<uint8_t> file_bytes = writer.build();
    REQUIRE_FALSE(file_bytes.empty());

    auto reader = ContainerReader::open(file_bytes); // throws if checksum is wrong

    CHECK(reader.header().format_major == kFormatMajor);
    CHECK(reader.manifest().document_id == m.document_id);
    REQUIRE(reader.manifest().blocks.size() == 1);

    auto block_payload = reader.read_block(block_type::kMetadata);
    REQUIRE(block_payload.has_value());

    model::Metadata m2 = serde::deserialize_metadata(*block_payload);
    CHECK(m2 == m);
}

TEST_CASE("container: uncompressed block also round-trips") {
    model::Metadata m = make_sample_metadata();

    ContainerWriter writer;
    writer.set_document_id(m.document_id);
    auto payload = serde::serialize_metadata(m);
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      payload, /*compress=*/false);

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);
    auto block_payload = reader.read_block(block_type::kMetadata);
    REQUIRE(block_payload.has_value());
    CHECK(serde::deserialize_metadata(*block_payload) == m);
}

TEST_CASE("container: corrupted checksum is detected") {
    model::Metadata m = make_sample_metadata();
    ContainerWriter writer;
    writer.set_document_id(m.document_id);
    auto payload = serde::serialize_metadata(m);
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion, payload);

    auto file_bytes = writer.build();
    file_bytes[file_bytes.size() / 2] ^= 0xFF; // flip a byte somewhere in the middle

    CHECK_THROWS_AS(ContainerReader::open(file_bytes), std::runtime_error);
}

TEST_CASE("container: missing block type returns nullopt, not an error") {
    model::Metadata m = make_sample_metadata();
    ContainerWriter writer;
    writer.set_document_id(m.document_id);
    auto payload = serde::serialize_metadata(m);
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion, payload);

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    auto missing = reader.read_block(block_type::kStyles);
    CHECK_FALSE(missing.has_value());
}

TEST_CASE("container: multiple blocks in one file round-trip independently") {
    ContainerWriter writer;
    writer.set_document_id("multi-block-doc");

    model::Metadata m1 = make_sample_metadata();
    m1.document_id = "multi-block-doc";
    auto payload1 = serde::serialize_metadata(m1);
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion, payload1);

    // A second, unrelated block using a made-up type_id/payload to prove
    // multiple blocks coexist and are individually addressable -- stands
    // in for Theme/Styles/etc. before their serde exists.
    std::vector<uint8_t> fake_theme_payload = {1, 2, 3, 4, 5};
    writer.add_block(block_type::kTheme, "theme", 1, fake_theme_payload);

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 2);

    auto metadata_payload = reader.read_block(block_type::kMetadata);
    REQUIRE(metadata_payload.has_value());
    CHECK(serde::deserialize_metadata(*metadata_payload) == m1);

    auto theme_payload = reader.read_block(block_type::kTheme);
    REQUIRE(theme_payload.has_value());
    CHECK(*theme_payload == fake_theme_payload);
}

TEST_CASE("container: realistic multi-block document (metadata + theme + styles)") {
    ContainerWriter writer;
    writer.set_document_id("realistic-doc");
    writer.set_created_by("idoc-engine-tests");

    model::Metadata m = make_sample_metadata();
    m.document_id = "realistic-doc";
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      serde::serialize_metadata(m));

    model::Theme theme;
    theme.colors.accent1 = {0x4A, 0x90, 0xD9, 255};
    theme.fonts.body = {"Calibri", "Arial"};
    theme.fonts.heading_major = {"Calibri Light", std::nullopt};
    writer.add_block(block_type::kTheme, "theme", serde::kThemeSchemaVersion,
                      serde::serialize_theme(theme));

    model::Styles styles;
    model::StyleDefinition normal;
    normal.style_id = "Normal";
    normal.display_name = "Normal";
    normal.type = model::StyleType::kParagraph;
    normal.is_default = true;
    styles.definitions.push_back(normal);

    model::StyleDefinition heading1;
    heading1.style_id = "Heading1";
    heading1.display_name = "Heading 1";
    heading1.type = model::StyleType::kParagraph;
    heading1.based_on = "Normal";
    styles.definitions.push_back(heading1);

    writer.add_block(block_type::kStyles, "styles", serde::kStylesSchemaVersion,
                      serde::serialize_styles(styles));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 3);

    auto m2 = serde::deserialize_metadata(*reader.read_block(block_type::kMetadata));
    CHECK(m2 == m);

    auto theme2 = serde::deserialize_theme(*reader.read_block(block_type::kTheme));
    CHECK(theme2 == theme);

    auto styles2 = serde::deserialize_styles(*reader.read_block(block_type::kStyles));
    CHECK(styles2 == styles);
    REQUIRE(styles2.definitions.size() == 2);
    CHECK(styles2.definitions[1].based_on.value() == "Normal");
}

TEST_CASE("container: four-block document (metadata + theme + styles + sections)") {
    ContainerWriter writer;
    writer.set_document_id("full-doc");

    model::Metadata m = make_sample_metadata();
    m.document_id = "full-doc";
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      serde::serialize_metadata(m));

    model::Theme theme;
    theme.fonts.body = {"Calibri", std::nullopt};
    writer.add_block(block_type::kTheme, "theme", serde::kThemeSchemaVersion,
                      serde::serialize_theme(theme));

    model::Styles styles;
    model::StyleDefinition normal;
    normal.style_id = "Normal";
    normal.display_name = "Normal";
    normal.type = model::StyleType::kParagraph;
    styles.definitions.push_back(normal);
    writer.add_block(block_type::kStyles, "styles", serde::kStylesSchemaVersion,
                      serde::serialize_styles(styles));

    model::Sections sections;
    model::Section sec;
    sec.section_id = "sec-1";
    sec.page_setup.page_size = {12240, 15840};
    sec.page_setup.margins = {1440, 1440, 1440, 1440, 0};
    sections.sections.push_back(sec);
    writer.add_block(block_type::kSections, "sections", serde::kSectionsSchemaVersion,
                      serde::serialize_sections(sections));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 4);

    CHECK(serde::deserialize_metadata(*reader.read_block(block_type::kMetadata)) == m);
    CHECK(serde::deserialize_theme(*reader.read_block(block_type::kTheme)) == theme);
    CHECK(serde::deserialize_styles(*reader.read_block(block_type::kStyles)) == styles);

    auto sections2 = serde::deserialize_sections(*reader.read_block(block_type::kSections));
    CHECK(sections2 == sections);
    REQUIRE(sections2.sections.size() == 1);
    CHECK(sections2.sections[0].page_setup.page_size.width == 12240);
}

TEST_CASE("container: six-block document (adds numbering + document content)") {
    ContainerWriter writer;
    writer.set_document_id("six-block-doc");

    model::Metadata m = make_sample_metadata();
    m.document_id = "six-block-doc";
    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      serde::serialize_metadata(m));

    model::Theme theme;
    writer.add_block(block_type::kTheme, "theme", serde::kThemeSchemaVersion,
                      serde::serialize_theme(theme));

    model::Styles styles;
    model::StyleDefinition normal;
    normal.style_id = "Normal";
    normal.display_name = "Normal";
    styles.definitions.push_back(normal);
    writer.add_block(block_type::kStyles, "styles", serde::kStylesSchemaVersion,
                      serde::serialize_styles(styles));

    model::Sections sections;
    model::Section sec;
    sec.section_id = "sec-1";
    sections.sections.push_back(sec);
    writer.add_block(block_type::kSections, "sections", serde::kSectionsSchemaVersion,
                      serde::serialize_sections(sections));

    model::NumberingDefinitions numbering;
    model::AbstractNum an;
    an.abstract_num_id = "abstract-1";
    for (size_t i = 0; i < an.levels.size(); ++i) an.levels[i].level = static_cast<int32_t>(i);
    numbering.abstract_nums.push_back(an);
    writer.add_block(block_type::kNumberingDefinitions, "numbering", serde::kNumberingSchemaVersion,
                      serde::serialize_numbering_definitions(numbering));

    model::DocumentContent content;
    model::Paragraph p;
    p.paragraph_id = "para-1";
    p.list_ref = model::ListRef{"list-instance-1", 0};
    model::Run r;
    r.run_id = "run-1";
    r.text = "Chapter One";
    p.runs.push_back(r);
    content.paragraphs.push_back(p);
    writer.add_block(block_type::kDocumentContent, "content", serde::kParagraphsSchemaVersion,
                      serde::serialize_document_content(content));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 6);

    CHECK(serde::deserialize_metadata(*reader.read_block(block_type::kMetadata)) == m);
    CHECK(serde::deserialize_theme(*reader.read_block(block_type::kTheme)) == theme);
    CHECK(serde::deserialize_styles(*reader.read_block(block_type::kStyles)) == styles);
    CHECK(serde::deserialize_sections(*reader.read_block(block_type::kSections)) == sections);

    auto numbering2 = serde::deserialize_numbering_definitions(*reader.read_block(block_type::kNumberingDefinitions));
    CHECK(numbering2 == numbering);

    auto content2 = serde::deserialize_document_content(*reader.read_block(block_type::kDocumentContent));
    CHECK(content2 == content);
    REQUIRE(content2.paragraphs.size() == 1);
    CHECK(content2.paragraphs[0].list_ref->instance_id == "list-instance-1");
    CHECK(content2.paragraphs[0].runs[0].text == "Chapter One");
}

TEST_CASE("container: eight-block document (adds fields + tables)") {
    ContainerWriter writer;
    writer.set_document_id("eight-block-doc");

    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      serde::serialize_metadata(model::Metadata{}));
    writer.add_block(block_type::kTheme, "theme", serde::kThemeSchemaVersion,
                      serde::serialize_theme(model::Theme{}));
    writer.add_block(block_type::kStyles, "styles", serde::kStylesSchemaVersion,
                      serde::serialize_styles(model::Styles{}));
    writer.add_block(block_type::kSections, "sections", serde::kSectionsSchemaVersion,
                      serde::serialize_sections(model::Sections{}));
    writer.add_block(block_type::kNumberingDefinitions, "numbering", serde::kNumberingSchemaVersion,
                      serde::serialize_numbering_definitions(model::NumberingDefinitions{}));
    writer.add_block(block_type::kDocumentContent, "content", serde::kParagraphsSchemaVersion,
                      serde::serialize_document_content(model::DocumentContent{}));

    model::Fields fields;
    model::Field field;
    field.field_id = "field-1";
    field.type = model::FieldType::kPageNumber;
    model::PageNumberFieldPayload pn;
    pn.pattern = "[Page]";
    field.page_number_payload = pn;
    field.cached_result = "1";
    field.cached_at = "2026-01-01T00:00:00Z";
    fields.fields.push_back(field);
    writer.add_block(block_type::kFields, "fields", serde::kFieldsSchemaVersion,
                      serde::serialize_fields(fields));

    model::Tables tables;
    model::Table table;
    table.table_id = "table-1";
    model::Row row;
    row.row_id = "row-1";
    model::Cell cell;
    cell.cell_id = "cell-1";
    row.cells.push_back(cell);
    table.rows.push_back(row);
    tables.tables.push_back(table);
    writer.add_block(block_type::kTables, "tables", serde::kTablesSchemaVersion,
                      serde::serialize_tables(tables));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 8);

    auto fields2 = serde::deserialize_fields(*reader.read_block(block_type::kFields));
    CHECK(fields2 == fields);
    REQUIRE(fields2.fields.size() == 1);
    CHECK(fields2.fields[0].page_number_payload->pattern == "[Page]");

    auto tables2 = serde::deserialize_tables(*reader.read_block(block_type::kTables));
    CHECK(tables2 == tables);
    REQUIRE(tables2.tables.size() == 1);
    CHECK(tables2.tables[0].rows[0].cells[0].cell_id == "cell-1");
}

TEST_CASE("container: twelve-block document (adds annotations + resources)") {
    ContainerWriter writer;
    writer.set_document_id("eleven-block-doc");

    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      serde::serialize_metadata(model::Metadata{}));
    writer.add_block(block_type::kTheme, "theme", serde::kThemeSchemaVersion,
                      serde::serialize_theme(model::Theme{}));
    writer.add_block(block_type::kStyles, "styles", serde::kStylesSchemaVersion,
                      serde::serialize_styles(model::Styles{}));
    writer.add_block(block_type::kSections, "sections", serde::kSectionsSchemaVersion,
                      serde::serialize_sections(model::Sections{}));
    writer.add_block(block_type::kNumberingDefinitions, "numbering", serde::kNumberingSchemaVersion,
                      serde::serialize_numbering_definitions(model::NumberingDefinitions{}));
    writer.add_block(block_type::kDocumentContent, "content", serde::kParagraphsSchemaVersion,
                      serde::serialize_document_content(model::DocumentContent{}));
    writer.add_block(block_type::kFields, "fields", serde::kFieldsSchemaVersion,
                      serde::serialize_fields(model::Fields{}));
    writer.add_block(block_type::kTables, "tables", serde::kTablesSchemaVersion,
                      serde::serialize_tables(model::Tables{}));

    model::FootnotesEndnotes fe;
    model::Note note;
    note.note_id = "fn-1";
    fe.footnotes.push_back(note);
    writer.add_block(block_type::kFootnotesEndnotes, "footnotes_endnotes", serde::kAnnotationsSchemaVersion,
                      serde::serialize_footnotes_endnotes(fe));

    model::Comments comments;
    model::Comment comment;
    comment.comment_id = "c1";
    comment.author = "Mystic";
    comment.created_at = "2026-09-06T00:00:00Z";
    comment.anchor_run_id = "run-1";
    comments.comments.push_back(comment);
    writer.add_block(block_type::kComments, "comments", serde::kAnnotationsSchemaVersion,
                      serde::serialize_comments(comments));

    model::BookmarksHyperlinks bh;
    model::Hyperlink link;
    link.hyperlink_id = "hl-1";
    link.target = "https://example.com";
    bh.hyperlinks.push_back(link);
    writer.add_block(block_type::kBookmarksHyperlinks, "bookmarks_hyperlinks", serde::kAnnotationsSchemaVersion,
                      serde::serialize_bookmarks_hyperlinks(bh));

    model::ResourceIndex idx;
    model::ResourceEntry entry;
    entry.resource_id = "res-1";
    entry.mime_type = "image/png";
    entry.sha256 = "deadbeef";
    idx.entries.push_back(entry);
    writer.add_block(block_type::kResourceIndex, "resources", serde::kResourcesSchemaVersion,
                      serde::serialize_resource_index(idx));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 12);

    auto fe2 = serde::deserialize_footnotes_endnotes(*reader.read_block(block_type::kFootnotesEndnotes));
    CHECK(fe2 == fe);

    auto comments2 = serde::deserialize_comments(*reader.read_block(block_type::kComments));
    CHECK(comments2 == comments);

    auto bh2 = serde::deserialize_bookmarks_hyperlinks(*reader.read_block(block_type::kBookmarksHyperlinks));
    CHECK(bh2 == bh);

    auto idx2 = serde::deserialize_resource_index(*reader.read_block(block_type::kResourceIndex));
    CHECK(idx2 == idx);
    REQUIRE(idx2.entries.size() == 1);
    CHECK(idx2.entries[0].mime_type == "image/png");
}

TEST_CASE("container: fourteen-block document (adds preserved_unknown + layout_cache)") {
    ContainerWriter writer;
    writer.set_document_id("fourteen-block-doc");

    writer.add_block(block_type::kMetadata, "metadata", serde::kMetadataSchemaVersion,
                      serde::serialize_metadata(model::Metadata{}));
    writer.add_block(block_type::kTheme, "theme", serde::kThemeSchemaVersion,
                      serde::serialize_theme(model::Theme{}));
    writer.add_block(block_type::kStyles, "styles", serde::kStylesSchemaVersion,
                      serde::serialize_styles(model::Styles{}));
    writer.add_block(block_type::kSections, "sections", serde::kSectionsSchemaVersion,
                      serde::serialize_sections(model::Sections{}));
    writer.add_block(block_type::kNumberingDefinitions, "numbering", serde::kNumberingSchemaVersion,
                      serde::serialize_numbering_definitions(model::NumberingDefinitions{}));
    writer.add_block(block_type::kDocumentContent, "content", serde::kParagraphsSchemaVersion,
                      serde::serialize_document_content(model::DocumentContent{}));
    writer.add_block(block_type::kFields, "fields", serde::kFieldsSchemaVersion,
                      serde::serialize_fields(model::Fields{}));
    writer.add_block(block_type::kTables, "tables", serde::kTablesSchemaVersion,
                      serde::serialize_tables(model::Tables{}));
    writer.add_block(block_type::kFootnotesEndnotes, "footnotes_endnotes", serde::kAnnotationsSchemaVersion,
                      serde::serialize_footnotes_endnotes(model::FootnotesEndnotes{}));
    writer.add_block(block_type::kComments, "comments", serde::kAnnotationsSchemaVersion,
                      serde::serialize_comments(model::Comments{}));
    writer.add_block(block_type::kBookmarksHyperlinks, "bookmarks_hyperlinks", serde::kAnnotationsSchemaVersion,
                      serde::serialize_bookmarks_hyperlinks(model::BookmarksHyperlinks{}));
    writer.add_block(block_type::kResourceIndex, "resources", serde::kResourcesSchemaVersion,
                      serde::serialize_resource_index(model::ResourceIndex{}));

    model::PreservedUnknown pu;
    pu.nodes.push_back(serde::make_preserved_node("para-1", "docx-ooxml", "/w:p/w:custom",
                                                   std::vector<uint8_t>{0xDE, 0xAD}));
    writer.add_block(block_type::kPreservedUnknown, "preserved_unknown", serde::kPreservedUnknownSchemaVersion,
                      serde::serialize_preserved_unknown(pu));

    model::LayoutCache lc;
    lc.generation = 3;
    model::PageGeometry page;
    page.page_number = 1;
    page.section_id = "sec-1";
    lc.pages.push_back(page);
    lc.field_results["field-1"] = "1";
    writer.add_block(block_type::kLayoutCache, "layout_cache", serde::kLayoutCacheSchemaVersion,
                      serde::serialize_layout_cache(lc));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    REQUIRE(reader.manifest().blocks.size() == 14);

    auto pu2 = serde::deserialize_preserved_unknown(*reader.read_block(block_type::kPreservedUnknown));
    CHECK(pu2 == pu);
    REQUIRE(pu2.nodes.size() == 1);
    CHECK(serde::verify_preserved_node_checksum(pu2.nodes[0]));

    auto lc2 = serde::deserialize_layout_cache(*reader.read_block(block_type::kLayoutCache));
    CHECK(lc2 == lc);
    CHECK(lc2.generation == 3);
}

TEST_CASE("container: Section.content ContentRefs resolve against Document Content across blocks") {
    // The point of the whole reference-format resolution: a Section's
    // ContentRefs should resolve to real Paragraph data stored in a
    // completely separate block, without the Sections block ever
    // embedding that data itself.
    ContainerWriter writer;
    writer.set_document_id("cross-block-resolution-doc");

    model::DocumentContent content;
    model::Paragraph p1;
    p1.paragraph_id = "para-1";
    model::Run r1;
    r1.run_id = "run-1";
    r1.text = "Chapter One";
    p1.runs.push_back(r1);
    content.paragraphs.push_back(p1);

    model::Paragraph p2;
    p2.paragraph_id = "para-2";
    model::Run r2;
    r2.run_id = "run-2";
    r2.text = "It was a dark and stormy night.";
    p2.runs.push_back(r2);
    content.paragraphs.push_back(p2);

    writer.add_block(block_type::kDocumentContent, "content", serde::kParagraphsSchemaVersion,
                      serde::serialize_document_content(content));

    model::Tables tables;
    model::Table table;
    table.table_id = "table-1";
    tables.tables.push_back(table);
    writer.add_block(block_type::kTables, "tables", serde::kTablesSchemaVersion,
                      serde::serialize_tables(tables));

    model::Sections sections;
    model::Section sec;
    sec.section_id = "sec-1";
    sec.content = {
        model::ContentRef{model::ContentType::kParagraph, "para-1"},
        model::ContentRef{model::ContentType::kTable, "table-1"},
        model::ContentRef{model::ContentType::kParagraph, "para-2"},
    };
    sections.sections.push_back(sec);
    writer.add_block(block_type::kSections, "sections", serde::kSectionsSchemaVersion,
                      serde::serialize_sections(sections));

    auto file_bytes = writer.build();
    auto reader = ContainerReader::open(file_bytes);

    auto sections2 = serde::deserialize_sections(*reader.read_block(block_type::kSections));
    auto content2 = serde::deserialize_document_content(*reader.read_block(block_type::kDocumentContent));

    REQUIRE(sections2.sections.size() == 1);
    const auto& refs = sections2.sections[0].content;
    REQUIRE(refs.size() == 3);

    // Resolve each ref manually, exactly as a real reader/renderer would.
    CHECK(refs[0].type == model::ContentType::kParagraph);
    auto find_paragraph = [&](const std::string& id) -> const model::Paragraph* {
        for (const auto& p : content2.paragraphs) if (p.paragraph_id == id) return &p;
        return nullptr;
    };
    const model::Paragraph* resolved1 = find_paragraph(refs[0].content_id);
    REQUIRE(resolved1 != nullptr);
    CHECK(resolved1->runs[0].text == "Chapter One");

    CHECK(refs[1].type == model::ContentType::kTable);
    CHECK(refs[1].content_id == "table-1");

    const model::Paragraph* resolved3 = find_paragraph(refs[2].content_id);
    REQUIRE(resolved3 != nullptr);
    CHECK(resolved3->runs[0].text == "It was a dark and stormy night.");
}
