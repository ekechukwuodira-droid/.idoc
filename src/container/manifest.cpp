#include "idoc/container/manifest.hpp"

#include <nlohmann/json.hpp>

namespace idoc {

using nlohmann::json;

namespace {

void to_json(json& j, const ManifestBlockEntry& b) {
    j = json{
        {"id", b.id},
        {"type", b.type},
        {"version", b.version},
        {"offset", b.offset},
        {"length", b.length},
    };
}

void from_json(const json& j, ManifestBlockEntry& b) {
    j.at("id").get_to(b.id);
    j.at("type").get_to(b.type);
    j.at("version").get_to(b.version);
    j.at("offset").get_to(b.offset);
    j.at("length").get_to(b.length);
}

} // namespace

std::string Manifest::to_json() const {
    json j;
    j["format_version"] = format_version;
    j["created_by"] = created_by;
    j["created_at"] = created_at;
    j["last_modified_by"] = last_modified_by;
    j["document_id"] = document_id;

    json blocks_json = json::array();
    for (const auto& b : blocks) {
        json bj;
        idoc::to_json(bj, b);
        blocks_json.push_back(bj);
    }
    j["blocks"] = blocks_json;
    j["resource_count"] = resource_count;
    j["incremental_save_generation"] = incremental_save_generation;

    return j.dump(2);
}

Manifest Manifest::from_json(const std::string& json_text) {
    json j = json::parse(json_text); // throws nlohmann::json::parse_error on malformed input

    Manifest m;
    j.at("format_version").get_to(m.format_version);
    j.at("created_by").get_to(m.created_by);
    j.at("created_at").get_to(m.created_at);
    j.at("last_modified_by").get_to(m.last_modified_by);
    j.at("document_id").get_to(m.document_id);

    for (const auto& bj : j.at("blocks")) {
        ManifestBlockEntry b;
        idoc::from_json(bj, b);
        m.blocks.push_back(b);
    }

    j.at("resource_count").get_to(m.resource_count);
    j.at("incremental_save_generation").get_to(m.incremental_save_generation);

    return m;
}

} // namespace idoc
