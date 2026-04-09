#include "Battle/Abilities.h"

#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
std::mutex gAbilityCacheMutex;

const std::vector<std::string> kAbilityJsonPaths = {
    "data/abilities.json",
    "../data/abilities.json",
    "../../data/abilities.json"
};

const std::vector<std::string> kSpeciesJsonPaths = {
    "data/species.json",
    "../data/species.json",
    "../../data/species.json"
};

const std::vector<std::string> kPokemonJsonRoots = {
    "data",
    "../data",
    "../../data"
};

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return s;
}

std::string normalizeName(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (char ch : name) {
        if (ch == ' ' || ch == '-' || ch == '\'' || ch == '_') {
            continue;
        }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

std::string slugToDisplayName(const std::string& slug) {
    std::string result;
    bool upper = true;
    for (char ch : slug) {
        if (ch == '-') {
            result.push_back(' ');
            upper = true;
            continue;
        }
        if (upper && std::isalpha(static_cast<unsigned char>(ch))) {
            result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
            upper = false;
        } else {
            result.push_back(ch);
            upper = false;
        }
    }
    return result;
}

AbilityType parseAbilityType(const std::string& value) {
    return getAbilityTypeByName(value);
}

AbilityData fallbackAbilityDataById(int id) {
    switch (id) {
        // 兼容旧本地测试数据 ID。
        case 1: return {1, "Blaze", "blaze", "When HP is below 1/3, Fire-type moves do 1.5x damage", AbilityType::Blaze};
        case 2: return {2, "Torrent", "torrent", "When HP is below 1/3, Water-type moves do 1.5x damage", AbilityType::Torrent};
        case 3: return {3, "Overgrow", "overgrow", "When HP is below 1/3, Grass-type moves do 1.5x damage", AbilityType::Overgrow};
        case 4: return {4, "Intimidate", "intimidate", "Lowers the foe's Attack stat on entry", AbilityType::Intimidate};
        case 5: return {5, "Multiscale", "multiscale", "Reduces damage when at full HP", AbilityType::Multiscale};

        // PokeAPI canonical IDs for abilities currently implemented by battle logic.
        case 9: return {9, "Static", "static", "Contact with the Pokemon may cause paralysis.", AbilityType::Static};
        case 10: return {10, "Volt Absorb", "volt-absorb", "Restores HP if hit by an Electric-type move.", AbilityType::VoltAbsorb};
        case 11: return {11, "Water Absorb", "water-absorb", "Restores HP if hit by a Water-type move.", AbilityType::WaterAbsorb};
        case 18: return {18, "Flash Fire", "flash-fire", "Powers up Fire-type moves if hit by one.", AbilityType::FlashFire};
        case 22: return {22, "Intimidate", "intimidate", "Lowers the foe's Attack stat on entry", AbilityType::Intimidate};
        case 26: return {26, "Levitate", "levitate", "Gives full immunity to Ground-type moves.", AbilityType::Levitate};
        case 65: return {65, "Overgrow", "overgrow", "When HP is below 1/3, Grass-type moves do 1.5x damage", AbilityType::Overgrow};
        case 66: return {66, "Blaze", "blaze", "When HP is below 1/3, Fire-type moves do 1.5x damage", AbilityType::Blaze};
        case 67: return {67, "Torrent", "torrent", "When HP is below 1/3, Water-type moves do 1.5x damage", AbilityType::Torrent};
        case 136: return {136, "Multiscale", "multiscale", "Reduces damage when at full HP", AbilityType::Multiscale};
        default: return {0, "None", "none", "", AbilityType::None};
    }
}

std::string resolveReadablePath(const std::vector<std::string>& paths) {
    for (const std::string& path : paths) {
        std::ifstream f(path);
        if (f.is_open()) {
            return path;
        }
    }
    return paths.front();
}

json loadAbilityRoot() {
    const std::string path = resolveReadablePath(kAbilityJsonPaths);
    std::ifstream file(path);
    if (!file.is_open()) {
        return json{{"abilities", json::array()}};
    }
    json root;
    file >> root;
    if (!root.contains("abilities") || !root["abilities"].is_array()) {
        root["abilities"] = json::array();
    }
    return root;
}

AbilityData parseAbilityEntry(const json& entry) {
    AbilityData data;
    data.id = entry.value("id", 0);
    data.name = entry.value("name", "");
    data.apiName = entry.value("apiName", "");
    data.description = entry.value("description", "");

    if (data.apiName.empty() && !data.name.empty()) {
        std::string api;
        api.reserve(data.name.size());
        for (char c : data.name) {
            api.push_back(c == ' ' ? '-' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
        data.apiName = api;
    }
    if (data.name.empty() && !data.apiName.empty()) {
        data.name = slugToDisplayName(data.apiName);
    }

    data.type = parseAbilityType(entry.value("mappedType", data.apiName));
    if (data.type == AbilityType::None) {
        data.type = parseAbilityType(data.name);
    }

    if (data.type == AbilityType::None) {
        AbilityData legacy = fallbackAbilityDataById(data.id);
        if (legacy.id != 0) {
            data.type = legacy.type;
        }
    }

    return data;
}

size_t curlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    std::string* out = static_cast<std::string*>(userp);
    out->append(static_cast<char*>(contents), total);
    return total;
}

bool fetchAbilityPayload(int id, json& outPayload) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    std::string response;
    const std::string url = "https://pokeapi.co/api/v2/ability/" + std::to_string(id);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "PokemonSimulator/1.0");

    CURLcode res = curl_easy_perform(curl);
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || statusCode != 200) {
        return false;
    }

    outPayload = json::parse(response, nullptr, false);
    return !outPayload.is_discarded();
}

AbilityData abilityDataFromPayload(const json& payload) {
    AbilityData data;
    data.id = payload.value("id", 0);
    data.apiName = payload.value("name", "");
    data.name = slugToDisplayName(data.apiName);
    data.description = "";

    if (payload.contains("effect_entries") && payload["effect_entries"].is_array()) {
        for (const auto& entry : payload["effect_entries"]) {
            if (entry.value("language", json::object()).value("name", "") == "en") {
                data.description = entry.value("short_effect", entry.value("effect", ""));
                break;
            }
        }
    }

    data.type = parseAbilityType(data.apiName);
    if (data.type == AbilityType::None) {
        AbilityData fallback = fallbackAbilityDataById(data.id);
        if (fallback.id != 0) {
            data.type = fallback.type;
            data.name = fallback.name;
            if (data.description.empty()) {
                data.description = fallback.description;
            }
        }
    }

    return data;
}

bool writeAbilityRootAtomically(const std::string& path, const json& root) {
    const std::string tempPath = path + ".tmp";
    {
        std::ofstream out(tempPath, std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }
        out << root.dump(2);
    }

    std::error_code ec;
    std::filesystem::rename(tempPath, path, ec);
    if (!ec) {
        return true;
    }
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(tempPath, path, ec);
    return !ec;
}

std::string resolveWritableAbilityPath() {
    for (const std::string& path : kAbilityJsonPaths) {
        std::ifstream f(path);
        if (f.is_open()) {
            return path;
        }
    }
    return kAbilityJsonPaths.front();
}

bool upsertAbilityEntry(const AbilityData& data, const json& payload) {
    if (data.id <= 0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(gAbilityCacheMutex);
    json root = loadAbilityRoot();
    json entry;
    entry["id"] = data.id;
    entry["name"] = data.name;
    entry["apiName"] = data.apiName;
    entry["description"] = data.description;
    entry["mappedType"] = [data]() {
        switch (data.type) {
            case AbilityType::Blaze: return "blaze";
            case AbilityType::Torrent: return "torrent";
            case AbilityType::Overgrow: return "overgrow";
            case AbilityType::Intimidate: return "intimidate";
            case AbilityType::Multiscale: return "multiscale";
            case AbilityType::Levitate: return "levitate";
            case AbilityType::WaterAbsorb: return "water-absorb";
            case AbilityType::VoltAbsorb: return "volt-absorb";
            case AbilityType::FlashFire: return "flash-fire";
            case AbilityType::Static: return "static";
            default: return "none";
        }
    }();

    if (payload.contains("effect_entries")) {
        entry["effect_entries"] = payload["effect_entries"];
    }
    if (payload.contains("pokemon")) {
        entry["pokemon_count"] = static_cast<int>(payload["pokemon"].size());
    }

    bool replaced = false;
    for (auto& existing : root["abilities"]) {
        if (existing.value("id", -1) == data.id) {
            existing = entry;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        root["abilities"].push_back(entry);
    }

    std::sort(root["abilities"].begin(), root["abilities"].end(), [](const json& a, const json& b) {
        return a.value("id", 0) < b.value("id", 0);
    });

    return writeAbilityRootAtomically(resolveWritableAbilityPath(), root);
}

std::set<int> collectReferencedAbilityIds() {
    std::set<int> ids;

    for (const std::string& path : kSpeciesJsonPaths) {
        std::ifstream f(path);
        if (!f.is_open()) {
            continue;
        }
        json root;
        f >> root;
        if (root.contains("species") && root["species"].is_array()) {
            for (const auto& spec : root["species"]) {
                if (!spec.contains("abilities") || !spec["abilities"].is_array()) {
                    continue;
                }
                for (const auto& aid : spec["abilities"]) {
                    if (aid.is_number_integer()) {
                        ids.insert(aid.get<int>());
                    }
                }
            }
        }
        break;
    }

    // 对仅字符串能力名的队伍文件，先映射到本地能力类型，再映射回 canonical id。
    for (const std::string& rootPath : kPokemonJsonRoots) {
        std::error_code ec;
        if (!std::filesystem::exists(rootPath, ec)) {
            continue;
        }
        for (const auto& file : std::filesystem::directory_iterator(rootPath, ec)) {
            if (ec || !file.is_regular_file() || file.path().extension() != ".json") {
                continue;
            }
            std::ifstream in(file.path());
            if (!in.is_open()) {
                continue;
            }
            json obj;
            in >> obj;
            if (!obj.contains("ability") || !obj["ability"].is_string()) {
                continue;
            }
            AbilityData ability = getAbilityDataByName(obj["ability"].get<std::string>());
            if (ability.id > 0) {
                ids.insert(ability.id);
            }
        }
        break;
    }

    return ids;
}
}  // namespace

AbilityData getAbilityDataById(int id) {
    if (id <= 0) {
        return fallbackAbilityDataById(0);
    }

    json root = loadAbilityRoot();
    for (const auto& entry : root["abilities"]) {
        if (entry.value("id", -1) != id) {
            continue;
        }
        AbilityData data = parseAbilityEntry(entry);
        if (data.id == 0) {
            data.id = id;
        }
        if (data.type == AbilityType::None) {
            AbilityData fallback = fallbackAbilityDataById(id);
            if (fallback.id != 0) {
                return fallback;
            }
        }
        return data;
    }

    AbilityData fallback = fallbackAbilityDataById(id);
    if (fallback.id != 0) {
        return fallback;
    }
    return {0, "None", "none", "", AbilityType::None};
}

AbilityData getAbilityDataByName(const std::string& name) {
    const std::string normalized = normalizeName(name);
    json root = loadAbilityRoot();
    for (const auto& entry : root["abilities"]) {
        AbilityData data = parseAbilityEntry(entry);
        if (normalizeName(data.name) == normalized || normalizeName(data.apiName) == normalized) {
            return data;
        }
    }

    // 兜底：遍历常见已实现能力 ID。
    const int fallbackIds[] = {1, 2, 3, 4, 5, 9, 10, 11, 18, 22, 26, 65, 66, 67, 136};
    for (int id : fallbackIds) {
        AbilityData fallback = fallbackAbilityDataById(id);
        if (fallback.id > 0 && (normalizeName(fallback.name) == normalized || normalizeName(fallback.apiName) == normalized)) {
            return fallback;
        }
    }

    return {0, "None", "none", "", AbilityType::None};
}

AbilityType getAbilityTypeById(int id) {
    return getAbilityDataById(id).type;
}

AbilityType getAbilityTypeByNameFromData(const std::string& name) {
    return getAbilityDataByName(name).type;
}

AbilityData getAbilityData(AbilityType type) {
    json root = loadAbilityRoot();
    for (const auto& entry : root["abilities"]) {
        AbilityData data = parseAbilityEntry(entry);
        if (data.type == type && data.id > 0) {
            return data;
        }
    }

    switch (type) {
        case AbilityType::Blaze: return fallbackAbilityDataById(66);
        case AbilityType::Torrent: return fallbackAbilityDataById(67);
        case AbilityType::Overgrow: return fallbackAbilityDataById(65);
        case AbilityType::Intimidate: return fallbackAbilityDataById(22);
        case AbilityType::Multiscale: return fallbackAbilityDataById(136);
        case AbilityType::Levitate: return fallbackAbilityDataById(26);
        case AbilityType::WaterAbsorb: return fallbackAbilityDataById(11);
        case AbilityType::VoltAbsorb: return fallbackAbilityDataById(10);
        case AbilityType::FlashFire: return fallbackAbilityDataById(18);
        case AbilityType::Static: return fallbackAbilityDataById(9);
        default: return {0, "None", "none", "", AbilityType::None};
    }
}

bool prefetchAbilitiesFromPokeAPI(bool refreshExisting) {
    const std::set<int> abilityIds = collectReferencedAbilityIds();
    if (abilityIds.empty()) {
        return false;
    }

    bool success = true;
    for (int id : abilityIds) {
        AbilityData local = getAbilityDataById(id);
        const bool hasLocal = local.id == id && !local.name.empty() && local.name != "None";
        if (!refreshExisting && hasLocal) {
            continue;
        }

        json payload;
        if (!fetchAbilityPayload(id, payload)) {
            success = false;
            continue;
        }

        AbilityData fetched = abilityDataFromPayload(payload);
        if (fetched.id != id) {
            success = false;
            continue;
        }

        if (!upsertAbilityEntry(fetched, payload)) {
            success = false;
            continue;
        }
    }

    return success;
}