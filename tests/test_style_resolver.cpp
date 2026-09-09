#include <doctest/doctest.h>
#include "idoc/resolve/style_resolver.hpp"

using namespace idoc;
using namespace idoc::resolve;

namespace {

model::StyleDefinition make_style(const std::string& id, model::StyleType type,
                                   std::optional<std::string> based_on = std::nullopt) {
    model::StyleDefinition s;
    s.style_id = id;
    s.display_name = id;
    s.type = type;
    s.based_on = based_on;
    return s;
}

} // namespace

TEST_CASE("style resolver: no style_id, no direct_props -> pure engine defaults") {
    StyleResolver resolver(model::Styles{});
    auto pf = resolver.resolve_paragraph(std::nullopt, std::nullopt);
    CHECK(pf == ResolvedParagraphFormat{});

    auto rf = resolver.resolve_run(std::nullopt, std::nullopt);
    CHECK(rf == ResolvedRunFormat{});
}

TEST_CASE("style resolver: direct_props overrides everything, including a named style") {
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    model::ParagraphProperties normal_props;
    normal_props.alignment = model::Alignment::kJustify;
    normal.paragraph_props = normal_props;

    model::Styles styles;
    styles.definitions = {normal};
    StyleResolver resolver(styles);

    model::ParagraphProperties direct;
    direct.alignment = model::Alignment::kCenter; // explicit override

    auto pf = resolver.resolve_paragraph("Normal", direct);
    CHECK(pf.alignment == model::Alignment::kCenter); // direct wins, not the style's Justify
}

TEST_CASE("style resolver: unset direct_props fields fall through to the style") {
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    model::ParagraphProperties normal_props;
    normal_props.alignment = model::Alignment::kJustify;
    normal_props.keep_with_next = true;
    normal.paragraph_props = normal_props;

    model::Styles styles;
    styles.definitions = {normal};
    StyleResolver resolver(styles);

    model::ParagraphProperties direct;
    direct.alignment = model::Alignment::kCenter; // sets only alignment
    // keep_with_next deliberately left unset in direct_props

    auto pf = resolver.resolve_paragraph("Normal", direct);
    CHECK(pf.alignment == model::Alignment::kCenter);   // from direct
    CHECK(pf.keep_with_next == true);                   // fell through to the style
}

TEST_CASE("style resolver: multi-level based_on chain, nearest ancestor wins per-field") {
    model::StyleDefinition grandparent = make_style("Grandparent", model::StyleType::kParagraph);
    model::ParagraphProperties gp_props;
    gp_props.alignment = model::Alignment::kJustify;
    gp_props.keep_lines_together = true;
    grandparent.paragraph_props = gp_props;

    model::StyleDefinition parent = make_style("Parent", model::StyleType::kParagraph, "Grandparent");
    model::ParagraphProperties p_props;
    p_props.alignment = model::Alignment::kRight; // overrides grandparent's Justify
    parent.paragraph_props = p_props;

    model::StyleDefinition child = make_style("Child", model::StyleType::kParagraph, "Parent");
    // child sets nothing of its own -- pure pass-through

    model::Styles styles;
    styles.definitions = {grandparent, parent, child};
    StyleResolver resolver(styles);

    auto pf = resolver.resolve_paragraph("Child", std::nullopt);
    CHECK(pf.alignment == model::Alignment::kRight);        // from Parent, nearer than Grandparent
    CHECK(pf.keep_lines_together == true);                  // fell all the way through to Grandparent
}

TEST_CASE("style resolver: cyclic based_on does not infinite-loop") {
    model::StyleDefinition a = make_style("A", model::StyleType::kParagraph, "B");
    model::StyleDefinition b = make_style("B", model::StyleType::kParagraph, "A"); // cycle: A -> B -> A

    model::ParagraphProperties a_props;
    a_props.alignment = model::Alignment::kCenter;
    a.paragraph_props = a_props;

    model::Styles styles;
    styles.definitions = {a, b};
    StyleResolver resolver(styles);

    // Must return promptly (no hang) and still resolve what it saw before the cycle closed.
    auto pf = resolver.resolve_paragraph("A", std::nullopt);
    CHECK(pf.alignment == model::Alignment::kCenter);
}

TEST_CASE("style resolver: dangling based_on reference stops cleanly, does not throw") {
    model::StyleDefinition orphan = make_style("Orphan", model::StyleType::kParagraph, "DoesNotExist");
    model::ParagraphProperties props;
    props.alignment = model::Alignment::kRight;
    orphan.paragraph_props = props;

    model::Styles styles;
    styles.definitions = {orphan};
    StyleResolver resolver(styles);

    auto pf = resolver.resolve_paragraph("Orphan", std::nullopt);
    CHECK(pf.alignment == model::Alignment::kRight); // Orphan's own value still applies
}

TEST_CASE("style resolver: unknown style_id falls through to document default style") {
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    normal.is_default = true;
    model::ParagraphProperties normal_props;
    normal_props.alignment = model::Alignment::kJustify;
    normal.paragraph_props = normal_props;

    model::Styles styles;
    styles.definitions = {normal};
    StyleResolver resolver(styles);

    auto pf = resolver.resolve_paragraph("StyleThatDoesNotExist", std::nullopt);
    CHECK(pf.alignment == model::Alignment::kJustify); // via the default style, not engine default
}

TEST_CASE("style resolver: no style_id at all still picks up the document default style") {
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    normal.is_default = true;
    model::ParagraphProperties normal_props;
    normal_props.keep_with_next = true;
    normal.paragraph_props = normal_props;

    model::Styles styles;
    styles.definitions = {normal};
    StyleResolver resolver(styles);

    auto pf = resolver.resolve_paragraph(std::nullopt, std::nullopt);
    CHECK(pf.keep_with_next == true);
}

TEST_CASE("style resolver: default style is not double-applied when already in the chain") {
    // If the resolved chain already includes the default style, it must
    // not be walked a second time (which would be harmless for
    // correctness here, but the point is testing the de-dup logic
    // itself doesn't misbehave).
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    normal.is_default = true;
    model::ParagraphProperties normal_props;
    normal_props.alignment = model::Alignment::kJustify;
    normal.paragraph_props = normal_props;

    model::StyleDefinition heading = make_style("Heading1", model::StyleType::kParagraph, "Normal");

    model::Styles styles;
    styles.definitions = {normal, heading};
    StyleResolver resolver(styles);

    auto pf = resolver.resolve_paragraph("Heading1", std::nullopt);
    CHECK(pf.alignment == model::Alignment::kJustify); // reached via Heading1 -> Normal, once
}

TEST_CASE("style resolver: run resolution -- direct run properties win over everything") {
    model::RunProperties direct;
    direct.bold = true;

    StyleResolver resolver(model::Styles{});
    auto rf = resolver.resolve_run(std::nullopt, direct);
    CHECK(rf.bold == true);
}

TEST_CASE("style resolver: run resolution falls through to paragraph style's run_props") {
    model::StyleDefinition heading = make_style("Heading1", model::StyleType::kParagraph);
    model::RunProperties heading_run_props;
    heading_run_props.bold = true;
    heading_run_props.size_pt = 16.0f;
    heading.run_props = heading_run_props;

    model::Styles styles;
    styles.definitions = {heading};
    StyleResolver resolver(styles);

    // No run-level direct_props at all -- everything comes from the paragraph style.
    auto rf = resolver.resolve_run("Heading1", std::nullopt);
    CHECK(rf.bold == true);
    CHECK(rf.size_pt == doctest::Approx(16.0f));
}

TEST_CASE("style resolver: character style on the run takes priority over the paragraph style") {
    model::StyleDefinition heading = make_style("Heading1", model::StyleType::kParagraph);
    model::RunProperties heading_run_props;
    heading_run_props.bold = true;
    heading_run_props.italic = false;
    heading.run_props = heading_run_props;

    model::StyleDefinition emphasis = make_style("Emphasis", model::StyleType::kCharacter);
    model::RunProperties emphasis_props;
    emphasis_props.italic = true; // overrides the paragraph style's italic=false
    emphasis.run_props = emphasis_props;

    model::Styles styles;
    styles.definitions = {heading, emphasis};
    StyleResolver resolver(styles);

    model::RunProperties direct;
    direct.style_id = "Emphasis"; // run names a character style, sets no other fields directly

    auto rf = resolver.resolve_run("Heading1", direct);
    CHECK(rf.italic == true);  // from the character style, wins over the paragraph style
    CHECK(rf.bold == true);    // fell through to the paragraph style (character style didn't set it)
}

TEST_CASE("style resolver: find_style and find_default_style") {
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    normal.is_default = true;
    model::StyleDefinition heading = make_style("Heading1", model::StyleType::kParagraph, "Normal");

    model::Styles styles;
    styles.definitions = {normal, heading};
    StyleResolver resolver(styles);

    CHECK(resolver.find_style("Heading1") != nullptr);
    CHECK(resolver.find_style("Heading1")->display_name == "Heading1");
    CHECK(resolver.find_style("DoesNotExist") == nullptr);

    CHECK(resolver.find_default_style(model::StyleType::kParagraph) != nullptr);
    CHECK(resolver.find_default_style(model::StyleType::kParagraph)->style_id == "Normal");
    CHECK(resolver.find_default_style(model::StyleType::kCharacter) == nullptr); // none defined
}

TEST_CASE("style resolver: convenience overloads taking Paragraph/Run objects directly") {
    model::StyleDefinition normal = make_style("Normal", model::StyleType::kParagraph);
    model::ParagraphProperties normal_props;
    normal_props.alignment = model::Alignment::kCenter;
    normal.paragraph_props = normal_props;

    model::Styles styles;
    styles.definitions = {normal};
    StyleResolver resolver(styles);

    model::Paragraph p;
    p.paragraph_id = "p1";
    p.style_id = "Normal";

    model::Run r;
    r.run_id = "r1";
    r.text = "hello";

    auto pf = resolver.resolve_paragraph(p);
    CHECK(pf.alignment == model::Alignment::kCenter);

    auto rf = resolver.resolve_run(p, r); // no run_props anywhere -- pure engine defaults
    CHECK(rf == ResolvedRunFormat{});
}

TEST_CASE("style resolver: engine defaults match documented values") {
    ResolvedRunFormat defaults;
    CHECK(defaults.font.family == "Calibri");
    CHECK(defaults.size_pt == doctest::Approx(11.0f));
    CHECK(defaults.bold == false);
    CHECK(defaults.color == model::Color{0, 0, 0, 255});
    CHECK(defaults.highlight.a == 0); // transparent == no highlight

    ResolvedParagraphFormat pf_defaults;
    CHECK(pf_defaults.alignment == model::Alignment::kLeft);
    CHECK(pf_defaults.keep_with_next == false);
}
