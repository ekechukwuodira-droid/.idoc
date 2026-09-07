#pragma once
// Metadata — §3. Pure data, no I/O. serde/metadata_serde.hpp maps this
// to/from the on-disk TLV block.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace idoc::model {

struct CustomProperty {
    std::string key;
    std::string value;

    bool operator==(const CustomProperty& other) const {
        return key == other.key && value == other.value;
    }
};

struct Metadata {
    std::string document_id;                 // stable GUID, never changes across saves
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> subject;
    std::vector<std::string> keywords;
    std::string language;                    // BCP-47, e.g. "en-US"
    std::string created_at;                  // ISO 8601
    std::string modified_at;                 // ISO 8601
    std::string application_version;
    uint32_t revision_number = 0;
    std::vector<CustomProperty> custom;      // escape hatch for app-specific fields

    bool operator==(const Metadata& other) const {
        return document_id == other.document_id &&
               title == other.title &&
               author == other.author &&
               subject == other.subject &&
               keywords == other.keywords &&
               language == other.language &&
               created_at == other.created_at &&
               modified_at == other.modified_at &&
               application_version == other.application_version &&
               revision_number == other.revision_number &&
               custom == other.custom;
    }
};

} // namespace idoc::model
