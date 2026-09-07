#include <doctest/doctest.h>
#include "idoc/serde/numbering_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

namespace {

model::AbstractNum make_sample_abstract_num() {
    model::AbstractNum an;
    an.abstract_num_id = "abstract-1";

    // Real invariant: level should always equal array position. Set it
    // for every slot up front so the fixture itself is internally
    // consistent, then customize the first three.
    for (size_t i = 0; i < an.levels.size(); ++i) {
        an.levels[i].level = static_cast<int32_t>(i);
    }

    // Level 0: decimal, e.g. "1."
    an.levels[0].level = 0;
    an.levels[0].format = model::NumberFormat::kDecimal;
    an.levels[0].text_pattern = "%1.";
    an.levels[0].alignment = model::Alignment::kLeft;
    an.levels[0].indent = {720, 0, -360, 0};
    an.levels[0].start_at = 1;
    an.levels[0].restart_rule.kind = model::RestartRule::Kind::kContinuous;

    // Level 1: lower letter, e.g. "1.a"
    an.levels[1].level = 1;
    an.levels[1].format = model::NumberFormat::kLowerLetter;
    an.levels[1].text_pattern = "%1.%2";
    an.levels[1].alignment = model::Alignment::kLeft;
    an.levels[1].indent = {1440, 0, -360, 0};
    an.levels[1].start_at = 1;
    an.levels[1].restart_rule.kind = model::RestartRule::Kind::kRestartAfterHigherLevel;
    an.levels[1].restart_rule.level_index = 0;

    // Level 2: bullet, with a glyph
    an.levels[2].level = 2;
    an.levels[2].format = model::NumberFormat::kBullet;
    an.levels[2].text_pattern = "%3";
    an.levels[2].bullet_glyph = 0x2022; // U+2022 BULLET
    an.levels[2].alignment = model::Alignment::kLeft;

    // Levels 3-8 left at defaults (still exercises full-9 round-trip)

    return an;
}

} // namespace

TEST_CASE("numbering serde: full round-trip with mixed level formats") {
    model::NumberingDefinitions nd;
    nd.abstract_nums = {make_sample_abstract_num()};

    model::NumberingInstance inst;
    inst.instance_id = "list-instance-1";
    inst.abstract_num_id = "abstract-1";
    nd.instances = {inst};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    CHECK(nd2 == nd);
}

TEST_CASE("numbering serde: all 9 levels preserve their own level index") {
    model::AbstractNum an = make_sample_abstract_num();
    for (int i = 0; i < model::kNumberingLevelCount; ++i) {
        CHECK(an.levels[i].level == i); // sanity on the fixture itself
    }

    model::NumberingDefinitions nd;
    nd.abstract_nums = {an};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    REQUIRE(nd2.abstract_nums.size() == 1);
    for (int i = 0; i < model::kNumberingLevelCount; ++i) {
        CHECK(nd2.abstract_nums[0].levels[i].level == i);
    }
}

TEST_CASE("numbering serde: RestartAfterHigherLevel carries its level_index") {
    model::NumberingDefinitions nd;
    nd.abstract_nums = {make_sample_abstract_num()};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    const auto& lvl1 = nd2.abstract_nums[0].levels[1];
    CHECK(lvl1.restart_rule.kind == model::RestartRule::Kind::kRestartAfterHigherLevel);
    REQUIRE(lvl1.restart_rule.level_index.has_value());
    CHECK(lvl1.restart_rule.level_index.value() == 0);
}

TEST_CASE("numbering serde: bullet glyph round-trips as the exact codepoint") {
    model::NumberingDefinitions nd;
    nd.abstract_nums = {make_sample_abstract_num()};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    CHECK(nd2.abstract_nums[0].levels[2].bullet_glyph.value() == 0x2022);
}

TEST_CASE("numbering serde: instance with level overrides round-trips") {
    model::NumberingInstance inst;
    inst.instance_id = "list-instance-2";
    inst.abstract_num_id = "abstract-1";

    model::LevelOverride ov0;
    ov0.level = 0;
    ov0.start_at = 5; // "this one list starts at 5, not 1" -- the spec's own example
    model::LevelOverride ov1;
    ov1.level = 1;
    // start_at left unset for this override

    inst.overrides = std::vector<model::LevelOverride>{ov0, ov1};

    model::NumberingDefinitions nd;
    nd.instances = {inst};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    REQUIRE(nd2.instances.size() == 1);
    REQUIRE(nd2.instances[0].overrides.has_value());
    REQUIRE(nd2.instances[0].overrides->size() == 2);
    CHECK(nd2.instances[0].overrides->at(0).start_at.value() == 5);
    CHECK_FALSE(nd2.instances[0].overrides->at(1).start_at.has_value());
}

TEST_CASE("numbering serde: instance without overrides round-trips as nullopt") {
    model::NumberingInstance inst;
    inst.instance_id = "list-instance-3";
    inst.abstract_num_id = "abstract-1";
    // overrides deliberately left unset

    model::NumberingDefinitions nd;
    nd.instances = {inst};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    CHECK_FALSE(nd2.instances[0].overrides.has_value());
}

TEST_CASE("numbering serde: number_run_props on a level round-trips") {
    model::AbstractNum an = make_sample_abstract_num();
    model::RunProperties run_props;
    run_props.bold = true;
    run_props.color = model::Color{0xFF, 0x00, 0x00, 255};
    an.levels[0].number_run_props = run_props;

    model::NumberingDefinitions nd;
    nd.abstract_nums = {an};

    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);

    REQUIRE(nd2.abstract_nums[0].levels[0].number_run_props.has_value());
    CHECK(nd2.abstract_nums[0].levels[0].number_run_props->bold.value() == true);
    CHECK(nd2.abstract_nums[0].levels[0].number_run_props->color.value() == model::Color{0xFF, 0x00, 0x00, 255});
}

TEST_CASE("numbering serde: empty NumberingDefinitions round-trips") {
    model::NumberingDefinitions nd;
    auto payload = serde::serialize_numbering_definitions(nd);
    auto nd2 = serde::deserialize_numbering_definitions(payload);
    CHECK(nd2.abstract_nums.empty());
    CHECK(nd2.instances.empty());
}

TEST_CASE("numbering serde: unknown top-level record type is skipped, not fatal") {
    model::NumberingDefinitions nd;
    nd.abstract_nums = {make_sample_abstract_num()};
    auto payload = serde::serialize_numbering_definitions(nd);
    tlv::write_record(payload, 999, 1, {0xFF});

    auto nd2 = serde::deserialize_numbering_definitions(payload);
    REQUIRE(nd2.abstract_nums.size() == 1);
    CHECK(nd2.abstract_nums[0].abstract_num_id == "abstract-1");
}
