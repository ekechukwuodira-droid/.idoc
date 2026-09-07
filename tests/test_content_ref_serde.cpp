#include <doctest/doctest.h>
#include "idoc/serde/common_serde.hpp"

using namespace idoc;

TEST_CASE("content_ref: single ref round-trip (paragraph)") {
    model::ContentRef ref{model::ContentType::kParagraph, "para-1"};
    auto payload = serde::common::encode_content_ref(ref);
    auto ref2 = serde::common::decode_content_ref(payload);
    CHECK(ref2 == ref);
}

TEST_CASE("content_ref: single ref round-trip (table)") {
    model::ContentRef ref{model::ContentType::kTable, "table-7"};
    auto payload = serde::common::encode_content_ref(ref);
    auto ref2 = serde::common::decode_content_ref(payload);
    CHECK(ref2.type == model::ContentType::kTable);
    CHECK(ref2.content_id == "table-7");
}

TEST_CASE("content_ref: ordered list of mixed types preserves order") {
    std::vector<model::ContentRef> refs = {
        {model::ContentType::kParagraph, "para-1"},
        {model::ContentType::kTable, "table-1"},
        {model::ContentType::kParagraph, "para-2"},
    };

    auto payload = serde::common::encode_content_refs(refs);
    auto refs2 = serde::common::decode_content_refs(payload);

    REQUIRE(refs2.size() == 3);
    CHECK(refs2[0].type == model::ContentType::kParagraph);
    CHECK(refs2[0].content_id == "para-1");
    CHECK(refs2[1].type == model::ContentType::kTable);
    CHECK(refs2[1].content_id == "table-1");
    CHECK(refs2[2].type == model::ContentType::kParagraph);
    CHECK(refs2[2].content_id == "para-2");
}

TEST_CASE("content_ref: empty list round-trips as empty") {
    std::vector<model::ContentRef> refs;
    auto payload = serde::common::encode_content_refs(refs);
    auto refs2 = serde::common::decode_content_refs(payload);
    CHECK(refs2.empty());
}

TEST_CASE("content_ref: recursive nesting (table cell referencing another table) needs no special-casing") {
    // The whole point of ID-based references: a cell's content list can
    // include a ContentRef with type == kTable, pointing at another
    // table, with no different code path from a paragraph reference.
    std::vector<model::ContentRef> cell_content = {
        {model::ContentType::kTable, "nested-table-1"},
    };

    auto payload = serde::common::encode_content_refs(cell_content);
    auto decoded = serde::common::decode_content_refs(payload);

    REQUIRE(decoded.size() == 1);
    CHECK(decoded[0].type == model::ContentType::kTable);
    CHECK(decoded[0].content_id == "nested-table-1");
}
