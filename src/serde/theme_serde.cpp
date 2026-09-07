#include "idoc/serde/theme_serde.hpp"
#include "idoc/container/tlv.hpp"
#include "idoc/container/byteorder.hpp"
#include "idoc/serde/common_serde.hpp"

namespace idoc::serde {

namespace field {
// Top-level Theme fields.
constexpr uint32_t kColors = 1;
constexpr uint32_t kFonts = 2;
} // namespace field

namespace color_field {
// Fields nested within a `colors` record's payload.
constexpr uint32_t kAccent1 = 1;
constexpr uint32_t kAccent2 = 2;
constexpr uint32_t kAccent3 = 3;
constexpr uint32_t kAccent4 = 4;
constexpr uint32_t kAccent5 = 5;
constexpr uint32_t kAccent6 = 6;
constexpr uint32_t kText1 = 7;
constexpr uint32_t kText2 = 8;
constexpr uint32_t kBackground1 = 9;
constexpr uint32_t kBackground2 = 10;
constexpr uint32_t kHyperlink = 11;
constexpr uint32_t kFollowedHyperlink = 12;
} // namespace color_field

namespace font_field {
// Fields nested within a `fonts` record's payload.
constexpr uint32_t kHeadingMajor = 1;
constexpr uint32_t kHeadingMinor = 2;
constexpr uint32_t kBody = 3;
} // namespace font_field

namespace {

void write_color_field(std::vector<uint8_t>& out, uint32_t field_id, const model::Color& c) {
    tlv::write_record(out, field_id, kThemeSchemaVersion, common::encode_color(c));
}

} // namespace

std::vector<uint8_t> serialize_theme(const model::Theme& t) {
    std::vector<uint8_t> out;

    {
        std::vector<uint8_t> colors_payload;
        write_color_field(colors_payload, color_field::kAccent1, t.colors.accent1);
        write_color_field(colors_payload, color_field::kAccent2, t.colors.accent2);
        write_color_field(colors_payload, color_field::kAccent3, t.colors.accent3);
        write_color_field(colors_payload, color_field::kAccent4, t.colors.accent4);
        write_color_field(colors_payload, color_field::kAccent5, t.colors.accent5);
        write_color_field(colors_payload, color_field::kAccent6, t.colors.accent6);
        write_color_field(colors_payload, color_field::kText1, t.colors.text1);
        write_color_field(colors_payload, color_field::kText2, t.colors.text2);
        write_color_field(colors_payload, color_field::kBackground1, t.colors.background1);
        write_color_field(colors_payload, color_field::kBackground2, t.colors.background2);
        write_color_field(colors_payload, color_field::kHyperlink, t.colors.hyperlink);
        write_color_field(colors_payload, color_field::kFollowedHyperlink, t.colors.followed_hyperlink);
        tlv::write_record(out, field::kColors, kThemeSchemaVersion, colors_payload);
    }

    {
        std::vector<uint8_t> fonts_payload;
        tlv::write_record(fonts_payload, font_field::kHeadingMajor, kThemeSchemaVersion,
                           common::encode_font_ref(t.fonts.heading_major));
        tlv::write_record(fonts_payload, font_field::kHeadingMinor, kThemeSchemaVersion,
                           common::encode_font_ref(t.fonts.heading_minor));
        tlv::write_record(fonts_payload, font_field::kBody, kThemeSchemaVersion,
                           common::encode_font_ref(t.fonts.body));
        tlv::write_record(out, field::kFonts, kThemeSchemaVersion, fonts_payload);
    }

    return out;
}

model::Theme deserialize_theme(const std::vector<uint8_t>& payload) {
    model::Theme t;
    auto records = tlv::parse_records(payload);

    for (const auto& rec : records) {
        switch (rec.header.type_id) {
            case field::kColors: {
                auto color_records = tlv::parse_records(rec.payload);
                for (const auto& cr : color_records) {
                    model::Color c = common::decode_color(cr.payload);
                    switch (cr.header.type_id) {
                        case color_field::kAccent1: t.colors.accent1 = c; break;
                        case color_field::kAccent2: t.colors.accent2 = c; break;
                        case color_field::kAccent3: t.colors.accent3 = c; break;
                        case color_field::kAccent4: t.colors.accent4 = c; break;
                        case color_field::kAccent5: t.colors.accent5 = c; break;
                        case color_field::kAccent6: t.colors.accent6 = c; break;
                        case color_field::kText1: t.colors.text1 = c; break;
                        case color_field::kText2: t.colors.text2 = c; break;
                        case color_field::kBackground1: t.colors.background1 = c; break;
                        case color_field::kBackground2: t.colors.background2 = c; break;
                        case color_field::kHyperlink: t.colors.hyperlink = c; break;
                        case color_field::kFollowedHyperlink: t.colors.followed_hyperlink = c; break;
                        default: break; // unknown color field: skip
                    }
                }
                break;
            }
            case field::kFonts: {
                auto font_records = tlv::parse_records(rec.payload);
                for (const auto& fr : font_records) {
                    model::FontRef f = common::decode_font_ref(fr.payload);
                    switch (fr.header.type_id) {
                        case font_field::kHeadingMajor: t.fonts.heading_major = f; break;
                        case font_field::kHeadingMinor: t.fonts.heading_minor = f; break;
                        case font_field::kBody: t.fonts.body = f; break;
                        default: break; // unknown font field: skip
                    }
                }
                break;
            }
            default:
                break; // unknown top-level field: skip
        }
    }

    return t;
}

} // namespace idoc::serde
