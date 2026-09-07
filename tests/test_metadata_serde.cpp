#include <doctest/doctest.h>
#include "idoc/serde/metadata_serde.hpp"
#include "idoc/container/tlv.hpp"

using namespace idoc;

TEST_CASE("metadata serde: full round-trip") {
    model::Metadata m;
    m.document_id = "doc-123";
    m.title = "The War of Ash and Iron";
    m.author = "Mystic";
    m.subject = "Fantasy";
    m.keywords = {"war", "magic", "kingdoms"};
    m.language = "en-US";
    m.created_at = "2026-08-30T12:00:00Z";
    m.modified_at = "2026-09-01T08:30:00Z";
    m.application_version = "LightOffice 1.0.0";
    m.revision_number = 4;
    m.custom = {{"project_code", "AI-42"}, {"template", "novel-manuscript"}};

    auto payload = serde::serialize_metadata(m);
    auto m2 = serde::deserialize_metadata(payload);

    CHECK(m2 == m);
}

TEST_CASE("metadata serde: optional fields absent round-trip as nullopt") {
    model::Metadata m;
    m.document_id = "doc-minimal";
    m.language = "en-US";
    m.created_at = "2026-01-01T00:00:00Z";
    m.modified_at = "2026-01-01T00:00:00Z";
    m.application_version = "test/0.1";
    // title, author, subject deliberately left unset

    auto payload = serde::serialize_metadata(m);
    auto m2 = serde::deserialize_metadata(payload);

    CHECK_FALSE(m2.title.has_value());
    CHECK_FALSE(m2.author.has_value());
    CHECK_FALSE(m2.subject.has_value());
    CHECK(m2 == m);
}

TEST_CASE("metadata serde: unknown field id is skipped, not fatal") {
    model::Metadata m;
    m.document_id = "doc-fwd-compat";
    m.language = "en-US";
    m.created_at = "2026-01-01T00:00:00Z";
    m.modified_at = "2026-01-01T00:00:00Z";
    m.application_version = "test/0.1";

    auto payload = serde::serialize_metadata(m);

    // Simulate a future engine version appending a field this reader
    // doesn't know about (id 999) -- must not break deserialization of
    // everything else, per §1.4's forward-compatibility contract.
    tlv::write_record(payload, 999, 1, {0x01, 0x02, 0x03});

    auto m2 = serde::deserialize_metadata(payload);
    CHECK(m2.document_id == "doc-fwd-compat");
    CHECK(m2.language == "en-US");
}

TEST_CASE("metadata serde: empty keywords and custom properties round-trip") {
    model::Metadata m;
    m.document_id = "doc-empty-lists";
    m.language = "en-US";
    m.created_at = "x";
    m.modified_at = "x";
    m.application_version = "x";

    auto payload = serde::serialize_metadata(m);
    auto m2 = serde::deserialize_metadata(payload);

    CHECK(m2.keywords.empty());
    CHECK(m2.custom.empty());
}
