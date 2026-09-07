// idoc_cli — manual test harness for the container/model/serde stage.
// Usage:
//   idoc_cli create --output <file> [--title T] [--author A] [--language en-US]
//   idoc_cli dump <file>
//   idoc_cli verify <file>

#include "idoc/container/block_types.hpp"
#include "idoc/container/container_reader.hpp"
#include "idoc/container/container_writer.hpp"
#include "idoc/model/annotations.hpp"
#include "idoc/model/fields.hpp"
#include "idoc/model/layout_cache.hpp"
#include "idoc/model/metadata.hpp"
#include "idoc/model/numbering.hpp"
#include "idoc/model/paragraph.hpp"
#include "idoc/model/preserved_unknown.hpp"
#include "idoc/model/resources.hpp"
#include "idoc/model/sections.hpp"
#include "idoc/model/styles.hpp"
#include "idoc/model/tables.hpp"
#include "idoc/model/theme.hpp"
#include "idoc/serde/annotations_serde.hpp"
#include "idoc/serde/fields_serde.hpp"
#include "idoc/serde/layout_cache_serde.hpp"
#include "idoc/serde/metadata_serde.hpp"
#include "idoc/serde/numbering_serde.hpp"
#include "idoc/serde/paragraph_serde.hpp"
#include "idoc/serde/preserved_unknown_serde.hpp"
#include "idoc/serde/resources_serde.hpp"
#include "idoc/serde/sections_serde.hpp"
#include "idoc/serde/styles_serde.hpp"
#include "idoc/serde/tables_serde.hpp"
#include "idoc/serde/theme_serde.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {

std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open file: " + path);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
}

void write_file(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("cannot write file: " + path);
    f.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
}

std::optional<std::string> get_flag(const std::vector<std::string>& args, const std::string& name) {
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == name && i + 1 < args.size()) return args[i + 1];
    }
    return std::nullopt;
}

bool has_flag(const std::vector<std::string>& args, const std::string& name) {
    for (const auto& a : args) if (a == name) return true;
    return false;
}

idoc::model::Theme make_default_theme() {
    idoc::model::Theme t;
    t.colors.accent1 = {0x4A, 0x90, 0xD9, 255};
    t.colors.accent2 = {0xE8, 0x7A, 0x41, 255};
    t.colors.accent3 = {0x50, 0xC8, 0x78, 255};
    t.colors.accent4 = {0xC9, 0x4A, 0x4A, 255};
    t.colors.accent5 = {0x9B, 0x59, 0xB6, 255};
    t.colors.accent6 = {0xF1, 0xC4, 0x0F, 255};
    t.colors.text1 = {0x00, 0x00, 0x00, 255};
    t.colors.text2 = {0x40, 0x40, 0x40, 255};
    t.colors.background1 = {0xFF, 0xFF, 0xFF, 255};
    t.colors.background2 = {0xF5, 0xF5, 0xF5, 255};
    t.colors.hyperlink = {0x05, 0x63, 0xC1, 255};
    t.colors.followed_hyperlink = {0x95, 0x4F, 0x9C, 255};
    t.fonts.heading_major = {"Calibri Light", "Arial"};
    t.fonts.heading_minor = {"Calibri Light", "Arial"};
    t.fonts.body = {"Calibri", "Arial"};
    return t;
}

idoc::model::Styles make_default_styles() {
    idoc::model::Styles s;

    idoc::model::StyleDefinition normal;
    normal.style_id = "Normal";
    normal.display_name = "Normal";
    normal.type = idoc::model::StyleType::kParagraph;
    normal.is_default = true;
    normal.quick_style = true;
    s.definitions.push_back(normal);

    idoc::model::StyleDefinition heading1;
    heading1.style_id = "Heading1";
    heading1.display_name = "Heading 1";
    heading1.type = idoc::model::StyleType::kParagraph;
    heading1.based_on = "Normal";
    heading1.next_style = "Normal";
    heading1.quick_style = true;
    s.definitions.push_back(heading1);

    idoc::model::StyleDefinition emphasis;
    emphasis.style_id = "Emphasis";
    emphasis.display_name = "Emphasis";
    emphasis.type = idoc::model::StyleType::kCharacter;
    emphasis.based_on = "DefaultParagraphFont";
    s.definitions.push_back(emphasis);

    return s;
}

idoc::model::Sections make_default_sections() {
    idoc::model::Sections sections;

    idoc::model::Section s;
    s.section_id = "sec-1";
    s.page_setup.page_size = {12240, 15840}; // US Letter, twips
    s.page_setup.margins = {1440, 1440, 1440, 1440, 0};
    s.page_setup.orientation = idoc::model::Orientation::kPortrait;
    s.page_setup.columns = {1, 0, true, false};

    idoc::model::HeaderFooterContent default_footer;
    // content left empty -- this demo's footer has no paragraphs of its own
    s.footers.default_ = default_footer;

    // References into Document Content by paragraph_id, in document order --
    // see model/content_ref.hpp for why this is references, not embedding.
    s.content = {
        idoc::model::ContentRef{idoc::model::ContentType::kParagraph, "para-1"},
        idoc::model::ContentRef{idoc::model::ContentType::kParagraph, "para-2"},
    };

    sections.sections.push_back(s);
    return sections;
}

idoc::model::DocumentContent make_default_document_content() {
    idoc::model::DocumentContent dc;

    idoc::model::Paragraph heading;
    heading.paragraph_id = "para-1";
    heading.style_id = "Heading1";
    idoc::model::Run heading_run;
    heading_run.run_id = "run-1";
    heading_run.text = "The War of Ash and Iron";
    heading.runs.push_back(heading_run);
    dc.paragraphs.push_back(heading);

    idoc::model::Paragraph body;
    body.paragraph_id = "para-2";
    body.style_id = "Normal";
    idoc::model::Run body_run;
    body_run.run_id = "run-2";
    body_run.text = "In the shadow of the ash mountains, two kingdoms drew their blades.";
    body.runs.push_back(body_run);
    dc.paragraphs.push_back(body);

    return dc;
}

idoc::model::NumberingDefinitions make_default_numbering() {
    idoc::model::NumberingDefinitions nd;

    idoc::model::AbstractNum an;
    an.abstract_num_id = "abstract-1";
    for (size_t i = 0; i < an.levels.size(); ++i) an.levels[i].level = static_cast<int32_t>(i);
    an.levels[0].format = idoc::model::NumberFormat::kDecimal;
    an.levels[0].text_pattern = "%1.";
    an.levels[0].indent = {720, 0, -360, 0};
    nd.abstract_nums.push_back(an);

    idoc::model::NumberingInstance inst;
    inst.instance_id = "list-instance-1";
    inst.abstract_num_id = "abstract-1";
    nd.instances.push_back(inst);

    return nd;
}

idoc::model::Fields make_default_fields() {
    idoc::model::Fields fields;

    idoc::model::Field f;
    f.field_id = "field-1";
    f.type = idoc::model::FieldType::kPageNumber;

    idoc::model::PageNumberFieldPayload payload;
    payload.position = idoc::model::PageNumberPosition::kFooter;
    payload.format.kind = idoc::model::PageNumberFormat::Kind::kDecimal;
    payload.pattern = "[Page]";
    f.page_number_payload = payload;

    f.cached_result = "1";
    f.cached_at = "2026-01-01T00:00:00Z";
    fields.fields.push_back(f);

    return fields;
}

idoc::model::Tables make_default_tables() {
    idoc::model::Tables tables;

    idoc::model::Table t;
    t.table_id = "table-1";
    t.props.alignment = idoc::model::Alignment::kLeft;

    idoc::model::Column col1;
    col1.width_type = idoc::model::Column::WidthType::kFixed;
    col1.width_twips = 2400;
    idoc::model::Column col2 = col1;
    t.columns = {col1, col2};

    idoc::model::Row header;
    header.row_id = "row-1";
    header.is_header_row = true;
    idoc::model::Cell c1; c1.cell_id = "cell-1-1";
    idoc::model::Cell c2; c2.cell_id = "cell-1-2";
    header.cells = {c1, c2};
    t.rows.push_back(header);

    tables.tables.push_back(t);
    return tables;
}

idoc::model::BookmarksHyperlinks make_default_bookmarks_hyperlinks() {
    idoc::model::BookmarksHyperlinks bh;

    idoc::model::Hyperlink h;
    h.hyperlink_id = "hl-1";
    h.target = "https://example.com";
    bh.hyperlinks.push_back(h);

    return bh;
}

idoc::model::FootnotesEndnotes make_default_footnotes_endnotes() {
    idoc::model::FootnotesEndnotes fe;

    idoc::model::Note footnote;
    footnote.note_id = "fn-1";
    footnote.restart_rule = idoc::model::NoteRestartRule::kPerPage;
    fe.footnotes.push_back(footnote);

    return fe;
}

idoc::model::Comments make_default_comments() {
    idoc::model::Comments comments;

    idoc::model::Comment c;
    c.comment_id = "comment-1";
    c.author = "Mystic";
    c.created_at = "2026-09-06T00:00:00Z";
    c.anchor_run_id = "run-1";
    comments.comments.push_back(c);

    return comments;
}

idoc::model::PreservedUnknown make_default_preserved_unknown() {
    // No entries by default -- --with-defaults isn't importing a real
    // DOCX, so there's nothing genuinely foreign to preserve. Left as a
    // real, empty PreservedUnknown so the block still exists and
    // round-trips.
    return idoc::model::PreservedUnknown{};
}

idoc::model::LayoutCache make_default_layout_cache() {
    idoc::model::LayoutCache lc;
    lc.generation = 1;

    idoc::model::PageGeometry page;
    page.page_number = 1;
    page.section_id = "sec-1";
    page.field_ids = {"field-1"};
    lc.pages.push_back(page);

    lc.field_results["field-1"] = "1";

    return lc;
}

idoc::model::ResourceIndex make_default_resources() {
    idoc::model::ResourceIndex idx;
    // No entries by default -- --with-defaults has no actual image bytes
    // to reference. Left as a real, empty ResourceIndex so the block
    // still exists and round-trips.
    return idx;
}

int cmd_create(const std::vector<std::string>& args) {
    auto output = get_flag(args, "--output");
    if (!output) {
        std::cerr << "create requires --output <file>\n";
        return 1;
    }

    idoc::model::Metadata m;
    m.document_id = get_flag(args, "--id").value_or("00000000-0000-0000-0000-000000000000");
    m.title = get_flag(args, "--title");
    m.author = get_flag(args, "--author");
    m.language = get_flag(args, "--language").value_or("en-US");
    m.created_at = get_flag(args, "--created-at").value_or("2026-01-01T00:00:00Z");
    m.modified_at = m.created_at;
    m.application_version = "idoc-engine-cli/0.1";
    m.revision_number = 1;

    idoc::ContainerWriter writer;
    writer.set_document_id(m.document_id);
    writer.set_created_at(m.created_at);
    writer.set_created_by("idoc-engine-cli");
    writer.set_last_modified_by("idoc-engine-cli");

    auto payload = idoc::serde::serialize_metadata(m);
    writer.add_block(idoc::block_type::kMetadata, "metadata",
                      idoc::serde::kMetadataSchemaVersion, payload, /*compress=*/true);

    if (has_flag(args, "--with-defaults")) {
        auto theme = make_default_theme();
        auto theme_payload = idoc::serde::serialize_theme(theme);
        writer.add_block(idoc::block_type::kTheme, "theme",
                          idoc::serde::kThemeSchemaVersion, theme_payload, /*compress=*/true);

        auto styles = make_default_styles();
        auto styles_payload = idoc::serde::serialize_styles(styles);
        writer.add_block(idoc::block_type::kStyles, "styles",
                          idoc::serde::kStylesSchemaVersion, styles_payload, /*compress=*/true);

        auto sections = make_default_sections();
        auto sections_payload = idoc::serde::serialize_sections(sections);
        writer.add_block(idoc::block_type::kSections, "sections",
                          idoc::serde::kSectionsSchemaVersion, sections_payload, /*compress=*/true);

        auto numbering = make_default_numbering();
        auto numbering_payload = idoc::serde::serialize_numbering_definitions(numbering);
        writer.add_block(idoc::block_type::kNumberingDefinitions, "numbering",
                          idoc::serde::kNumberingSchemaVersion, numbering_payload, /*compress=*/true);

        auto content = make_default_document_content();
        auto content_payload = idoc::serde::serialize_document_content(content);
        writer.add_block(idoc::block_type::kDocumentContent, "content",
                          idoc::serde::kParagraphsSchemaVersion, content_payload, /*compress=*/true);

        auto fields = make_default_fields();
        auto fields_payload = idoc::serde::serialize_fields(fields);
        writer.add_block(idoc::block_type::kFields, "fields",
                          idoc::serde::kFieldsSchemaVersion, fields_payload, /*compress=*/true);

        auto tables = make_default_tables();
        auto tables_payload = idoc::serde::serialize_tables(tables);
        writer.add_block(idoc::block_type::kTables, "tables",
                          idoc::serde::kTablesSchemaVersion, tables_payload, /*compress=*/true);

        auto bh = make_default_bookmarks_hyperlinks();
        auto bh_payload = idoc::serde::serialize_bookmarks_hyperlinks(bh);
        writer.add_block(idoc::block_type::kBookmarksHyperlinks, "bookmarks_hyperlinks",
                          idoc::serde::kAnnotationsSchemaVersion, bh_payload, /*compress=*/true);

        auto fe = make_default_footnotes_endnotes();
        auto fe_payload = idoc::serde::serialize_footnotes_endnotes(fe);
        writer.add_block(idoc::block_type::kFootnotesEndnotes, "footnotes_endnotes",
                          idoc::serde::kAnnotationsSchemaVersion, fe_payload, /*compress=*/true);

        auto comments = make_default_comments();
        auto comments_payload = idoc::serde::serialize_comments(comments);
        writer.add_block(idoc::block_type::kComments, "comments",
                          idoc::serde::kAnnotationsSchemaVersion, comments_payload, /*compress=*/true);

        auto resources = make_default_resources();
        auto resources_payload = idoc::serde::serialize_resource_index(resources);
        writer.add_block(idoc::block_type::kResourceIndex, "resources",
                          idoc::serde::kResourcesSchemaVersion, resources_payload, /*compress=*/true);

        auto preserved = make_default_preserved_unknown();
        auto preserved_payload = idoc::serde::serialize_preserved_unknown(preserved);
        writer.add_block(idoc::block_type::kPreservedUnknown, "preserved_unknown",
                          idoc::serde::kPreservedUnknownSchemaVersion, preserved_payload, /*compress=*/true);

        auto layout_cache = make_default_layout_cache();
        auto layout_cache_payload = idoc::serde::serialize_layout_cache(layout_cache);
        writer.add_block(idoc::block_type::kLayoutCache, "layout_cache",
                          idoc::serde::kLayoutCacheSchemaVersion, layout_cache_payload, /*compress=*/true);
    }

    auto bytes = writer.build();
    write_file(*output, bytes);
    std::cout << "wrote " << bytes.size() << " bytes to " << *output << "\n";
    return 0;
}

int cmd_dump(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "dump requires a file path\n";
        return 1;
    }
    auto bytes = read_file(args[0]);
    auto reader = idoc::ContainerReader::open(bytes);

    const auto& h = reader.header();
    std::cout << "format: " << h.format_major << "." << h.format_minor << "\n";
    std::cout << "file_length (header claims): " << h.file_length
               << " (actual: " << bytes.size() << ")\n";
    std::cout << "manifest:\n" << reader.manifest().to_json() << "\n";

    auto metadata_payload = reader.read_block(idoc::block_type::kMetadata);
    if (metadata_payload) {
        auto m = idoc::serde::deserialize_metadata(*metadata_payload);
        std::cout << "\nmetadata:\n";
        std::cout << "  document_id: " << m.document_id << "\n";
        std::cout << "  title: " << m.title.value_or("(none)") << "\n";
        std::cout << "  author: " << m.author.value_or("(none)") << "\n";
        std::cout << "  language: " << m.language << "\n";
        std::cout << "  revision_number: " << m.revision_number << "\n";
    } else {
        std::cout << "\n(no metadata block found)\n";
    }

    auto theme_payload = reader.read_block(idoc::block_type::kTheme);
    if (theme_payload) {
        auto t = idoc::serde::deserialize_theme(*theme_payload);
        std::cout << "\ntheme:\n";
        std::cout << "  body font: " << t.fonts.body.family
                   << " (fallback: " << t.fonts.body.fallback.value_or("none") << ")\n";
        std::cout << "  heading font: " << t.fonts.heading_major.family << "\n";
        std::cout << "  accent1: rgba(" << (int)t.colors.accent1.r << "," << (int)t.colors.accent1.g
                   << "," << (int)t.colors.accent1.b << "," << (int)t.colors.accent1.a << ")\n";
    }

    auto styles_payload = reader.read_block(idoc::block_type::kStyles);
    if (styles_payload) {
        auto s = idoc::serde::deserialize_styles(*styles_payload);
        std::cout << "\nstyles (" << s.definitions.size() << "):\n";
        for (const auto& def : s.definitions) {
            std::cout << "  " << def.style_id << " (" << def.display_name << ")"
                       << (def.based_on ? " based_on=" + *def.based_on : "")
                       << (def.is_default ? " [default]" : "") << "\n";
        }
    }

    auto sections_payload = reader.read_block(idoc::block_type::kSections);
    if (sections_payload) {
        auto sections = idoc::serde::deserialize_sections(*sections_payload);

        // Resolve Section.content ContentRefs against Document Content, to
        // actually demonstrate the reference format working end-to-end
        // rather than just printing type/id pairs.
        std::map<std::string, std::string> paragraph_text_by_id;
        auto content_payload_for_resolution = reader.read_block(idoc::block_type::kDocumentContent);
        if (content_payload_for_resolution) {
            auto dc = idoc::serde::deserialize_document_content(*content_payload_for_resolution);
            for (const auto& p : dc.paragraphs) {
                std::string text;
                for (const auto& run : p.runs) text += run.text;
                paragraph_text_by_id[p.paragraph_id] = text;
            }
        }

        std::cout << "\nsections (" << sections.sections.size() << "):\n";
        for (const auto& sec : sections.sections) {
            std::cout << "  " << sec.section_id << ": "
                       << sec.page_setup.page_size.width << "x" << sec.page_setup.page_size.height
                       << " twips, "
                       << (sec.page_setup.orientation == idoc::model::Orientation::kPortrait ? "portrait" : "landscape")
                       << ", " << sec.page_setup.columns.count << " column(s)"
                       << (sec.headers.default_.has_value() ? ", has default header" : "")
                       << (sec.footers.default_.has_value() ? ", has default footer" : "")
                       << "\n";
            for (const auto& ref : sec.content) {
                std::cout << "    -> "
                           << (ref.type == idoc::model::ContentType::kParagraph ? "paragraph " : "table ")
                           << ref.content_id;
                auto it = paragraph_text_by_id.find(ref.content_id);
                if (it != paragraph_text_by_id.end()) {
                    std::cout << ": \"" << it->second << "\"";
                }
                std::cout << "\n";
            }
        }
    }

    auto numbering_payload = reader.read_block(idoc::block_type::kNumberingDefinitions);
    if (numbering_payload) {
        auto nd = idoc::serde::deserialize_numbering_definitions(*numbering_payload);
        std::cout << "\nnumbering (" << nd.abstract_nums.size() << " abstract, "
                   << nd.instances.size() << " instance(s)):\n";
        for (const auto& an : nd.abstract_nums) {
            std::cout << "  " << an.abstract_num_id << ": level0 pattern=\""
                       << an.levels[0].text_pattern << "\"\n";
        }
    }

    auto content_payload = reader.read_block(idoc::block_type::kDocumentContent);
    if (content_payload) {
        auto dc = idoc::serde::deserialize_document_content(*content_payload);
        std::cout << "\ncontent (" << dc.paragraphs.size() << " paragraph(s)):\n";
        for (const auto& p : dc.paragraphs) {
            std::string text;
            for (const auto& run : p.runs) text += run.text;
            std::cout << "  [" << p.paragraph_id << "] "
                       << (p.style_id ? *p.style_id + ": " : "")
                       << text << "\n";
        }
    }

    auto fields_payload = reader.read_block(idoc::block_type::kFields);
    if (fields_payload) {
        auto fields = idoc::serde::deserialize_fields(*fields_payload);
        std::cout << "\nfields (" << fields.fields.size() << "):\n";
        for (const auto& f : fields.fields) {
            std::cout << "  [" << f.field_id << "] cached=\"" << f.cached_result << "\"\n";
        }
    }

    auto tables_payload = reader.read_block(idoc::block_type::kTables);
    if (tables_payload) {
        auto tables = idoc::serde::deserialize_tables(*tables_payload);
        std::cout << "\ntables (" << tables.tables.size() << "):\n";
        for (const auto& t : tables.tables) {
            std::cout << "  [" << t.table_id << "] " << t.columns.size() << " column(s), "
                       << t.rows.size() << " row(s)\n";
        }
    }

    auto bh_payload = reader.read_block(idoc::block_type::kBookmarksHyperlinks);
    if (bh_payload) {
        auto bh = idoc::serde::deserialize_bookmarks_hyperlinks(*bh_payload);
        std::cout << "\nbookmarks (" << bh.bookmarks.size() << "), hyperlinks ("
                   << bh.hyperlinks.size() << "):\n";
        for (const auto& h : bh.hyperlinks) {
            std::cout << "  [" << h.hyperlink_id << "] -> " << h.target << "\n";
        }
    }

    auto fe_payload = reader.read_block(idoc::block_type::kFootnotesEndnotes);
    if (fe_payload) {
        auto fe = idoc::serde::deserialize_footnotes_endnotes(*fe_payload);
        std::cout << "\nfootnotes (" << fe.footnotes.size() << "), endnotes ("
                   << fe.endnotes.size() << ")\n";
    }

    auto comments_payload = reader.read_block(idoc::block_type::kComments);
    if (comments_payload) {
        auto comments = idoc::serde::deserialize_comments(*comments_payload);
        std::cout << "\ncomments (" << comments.comments.size() << "):\n";
        for (const auto& c : comments.comments) {
            std::cout << "  [" << c.comment_id << "] " << c.author << "\n";
        }
    }

    auto resources_payload = reader.read_block(idoc::block_type::kResourceIndex);
    if (resources_payload) {
        auto idx = idoc::serde::deserialize_resource_index(*resources_payload);
        std::cout << "\nresources (" << idx.entries.size() << "):\n";
        for (const auto& e : idx.entries) {
            std::cout << "  [" << e.resource_id << "] " << e.mime_type << "\n";
        }
    }

    auto preserved_payload = reader.read_block(idoc::block_type::kPreservedUnknown);
    if (preserved_payload) {
        auto pu = idoc::serde::deserialize_preserved_unknown(*preserved_payload);
        std::cout << "\npreserved_unknown nodes (" << pu.nodes.size() << ")\n";
    }

    auto layout_cache_payload = reader.read_block(idoc::block_type::kLayoutCache);
    if (layout_cache_payload) {
        auto lc = idoc::serde::deserialize_layout_cache(*layout_cache_payload);
        std::cout << "\nlayout_cache: generation=" << lc.generation
                   << ", " << lc.pages.size() << " page(s), "
                   << lc.field_results.size() << " cached field result(s)\n";
    }
    return 0;
}

int cmd_verify(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "verify requires a file path\n";
        return 1;
    }
    try {
        auto bytes = read_file(args[0]);
        idoc::ContainerReader::open(bytes); // throws on checksum/parse failure
        std::cout << "OK: checksum and structure valid\n";
        return 0;
    } catch (const std::exception& e) {
        std::cout << "INVALID: " << e.what() << "\n";
        return 1;
    }
}

} // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        std::cerr << "usage: idoc_cli <create|dump|verify> [args...]\n";
        return 1;
    }

    std::string cmd = args[0];
    std::vector<std::string> rest(args.begin() + 1, args.end());

    try {
        if (cmd == "create") return cmd_create(rest);
        if (cmd == "dump") return cmd_dump(rest);
        if (cmd == "verify") return cmd_verify(rest);
        std::cerr << "unknown command: " << cmd << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
}
