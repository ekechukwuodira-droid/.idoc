#include "idoc/resolve/style_resolver.hpp"

#include <unordered_set>

namespace idoc::resolve {

StyleResolver::StyleResolver(model::Styles styles) : styles_(std::move(styles)) {
    for (const auto& def : styles_.definitions) {
        by_id_[def.style_id] = &def;
    }
}

const model::StyleDefinition* StyleResolver::find_style(const std::string& style_id) const {
    auto it = by_id_.find(style_id);
    return it != by_id_.end() ? it->second : nullptr;
}

const model::StyleDefinition* StyleResolver::find_default_style(model::StyleType type) const {
    for (const auto& def : styles_.definitions) {
        if (def.is_default && def.type == type) return &def;
    }
    return nullptr;
}

std::vector<const model::StyleDefinition*> StyleResolver::resolve_chain(const std::string& style_id) const {
    std::vector<const model::StyleDefinition*> chain;
    std::unordered_set<std::string> visited;

    std::string current = style_id;
    while (!current.empty()) {
        if (visited.count(current) != 0) break; // cycle: stop at the point of repetition
        visited.insert(current);

        const model::StyleDefinition* def = find_style(current);
        if (def == nullptr) break; // dangling based_on reference: stop, don't throw

        chain.push_back(def);

        if (def->based_on.has_value()) {
            current = *def->based_on;
        } else {
            break;
        }
    }
    return chain;
}

ResolvedParagraphFormat StyleResolver::resolve_paragraph(
    const std::optional<std::string>& style_id,
    const std::optional<model::ParagraphProperties>& direct_props) const {

    ResolvedParagraphFormat result; // engine defaults

    std::vector<const model::ParagraphProperties*> candidates;
    if (direct_props.has_value()) candidates.push_back(&*direct_props);

    std::vector<const model::StyleDefinition*> chain;
    if (style_id.has_value()) chain = resolve_chain(*style_id);
    for (const auto* def : chain) {
        if (def->paragraph_props.has_value()) candidates.push_back(&*def->paragraph_props);
    }

    // Fall back to the document's default paragraph style, if one exists
    // and isn't already part of the chain we just walked.
    const model::StyleDefinition* default_style = find_default_style(model::StyleType::kParagraph);
    if (default_style != nullptr) {
        bool already_in_chain = false;
        for (const auto* def : chain) {
            if (def == default_style) { already_in_chain = true; break; }
        }
        if (!already_in_chain && default_style->paragraph_props.has_value()) {
            candidates.push_back(&*default_style->paragraph_props);
        }
    }

    // First candidate (in priority order) to set a given property wins
    // for that property specifically -- per-field resolution, not
    // per-object.
    bool has_alignment = false, has_indent = false, has_spacing = false;
    bool has_keep_with_next = false, has_keep_lines_together = false, has_page_break_before = false;

    for (const auto* props : candidates) {
        if (!has_alignment && props->alignment.has_value()) {
            result.alignment = *props->alignment;
            has_alignment = true;
        }
        if (!has_indent && props->indent.has_value()) {
            result.indent = *props->indent;
            has_indent = true;
        }
        if (!has_spacing && props->spacing.has_value()) {
            result.spacing = *props->spacing;
            has_spacing = true;
        }
        if (!has_keep_with_next && props->keep_with_next.has_value()) {
            result.keep_with_next = *props->keep_with_next;
            has_keep_with_next = true;
        }
        if (!has_keep_lines_together && props->keep_lines_together.has_value()) {
            result.keep_lines_together = *props->keep_lines_together;
            has_keep_lines_together = true;
        }
        if (!has_page_break_before && props->page_break_before.has_value()) {
            result.page_break_before = *props->page_break_before;
            has_page_break_before = true;
        }
    }

    return result;
}

ResolvedRunFormat StyleResolver::resolve_run(
    const std::optional<std::string>& paragraph_style_id,
    const std::optional<model::RunProperties>& run_direct_props) const {

    ResolvedRunFormat result; // engine defaults

    std::vector<const model::RunProperties*> candidates;
    if (run_direct_props.has_value()) candidates.push_back(&*run_direct_props);

    // A character style named directly on this run's own formatting.
    if (run_direct_props.has_value() && run_direct_props->style_id.has_value()) {
        auto char_chain = resolve_chain(*run_direct_props->style_id);
        for (const auto* def : char_chain) {
            if (def->run_props.has_value()) candidates.push_back(&*def->run_props);
        }
    }

    // The owning paragraph's own style chain -- a paragraph style also
    // carries default character formatting for text typed in it.
    if (paragraph_style_id.has_value()) {
        auto para_chain = resolve_chain(*paragraph_style_id);
        for (const auto* def : para_chain) {
            if (def->run_props.has_value()) candidates.push_back(&*def->run_props);
        }
    }

    // Document's default character style, if any.
    const model::StyleDefinition* default_char_style = find_default_style(model::StyleType::kCharacter);
    if (default_char_style != nullptr && default_char_style->run_props.has_value()) {
        candidates.push_back(&*default_char_style->run_props);
    }

    bool has_font = false, has_size = false, has_bold = false, has_italic = false;
    bool has_underline = false, has_strike = false, has_valign = false, has_color = false;
    bool has_highlight = false, has_charspacing = false, has_smallcaps = false, has_allcaps = false;

    for (const auto* rp : candidates) {
        if (!has_font && rp->font.has_value()) { result.font = *rp->font; has_font = true; }
        if (!has_size && rp->size_pt.has_value()) { result.size_pt = *rp->size_pt; has_size = true; }
        if (!has_bold && rp->bold.has_value()) { result.bold = *rp->bold; has_bold = true; }
        if (!has_italic && rp->italic.has_value()) { result.italic = *rp->italic; has_italic = true; }
        if (!has_underline && rp->underline.has_value()) {
            result.underline = *rp->underline;
            has_underline = true;
        }
        if (!has_strike && rp->strikethrough.has_value()) {
            result.strikethrough = *rp->strikethrough;
            has_strike = true;
        }
        if (!has_valign && rp->vertical_align.has_value()) {
            result.vertical_align = *rp->vertical_align;
            has_valign = true;
        }
        if (!has_color && rp->color.has_value()) { result.color = *rp->color; has_color = true; }
        if (!has_highlight && rp->highlight.has_value()) {
            result.highlight = *rp->highlight;
            has_highlight = true;
        }
        if (!has_charspacing && rp->character_spacing_pt.has_value()) {
            result.character_spacing_pt = *rp->character_spacing_pt;
            has_charspacing = true;
        }
        if (!has_smallcaps && rp->small_caps.has_value()) {
            result.small_caps = *rp->small_caps;
            has_smallcaps = true;
        }
        if (!has_allcaps && rp->all_caps.has_value()) {
            result.all_caps = *rp->all_caps;
            has_allcaps = true;
        }
    }

    return result;
}

} // namespace idoc::resolve
