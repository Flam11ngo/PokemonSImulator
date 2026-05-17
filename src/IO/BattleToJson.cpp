#include "IO/BattleToJson.h"
#include "battle/Types.h"
#include "battle/Natures.h"
#include "battle/Abilities.h"
#include "battle/Items.h"
#include "battle/Status.h"
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <cctype>
#include <sstream>
#include <unordered_map>

namespace {

struct PokemonRef {
    int sideIndex = -1;
    int pokemonIndex = -1;
};

std::string normalizeToken(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char ch : input) {
        if (ch == ' ' || ch == '_' || ch == '-' || ch == '\'') {
            continue;
        }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

std::unordered_map<std::string, PokemonRef> buildPokemonLookup(const Battle& battle) {
    std::unordered_map<std::string, PokemonRef> lookup;
    const Side* sides[2] = {&battle.getSideA(), &battle.getSideB()};
    for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
        const Side& side = *sides[sideIndex];
        const auto& team = side.getTeam();
        for (int i = 0; i < side.getPokemonCount(); ++i) {
            Pokemon* pokemon = team[i];
            if (!pokemon) {
                continue;
            }
            const std::string key = normalizeToken(pokemon->getName());
            if (lookup.find(key) == lookup.end()) {
                lookup[key] = PokemonRef{sideIndex, i};
            }
        }
    }
    return lookup;
}

json pokemonRefToJson(const std::unordered_map<std::string, PokemonRef>& lookup, const std::string& pokemonName) {
    const auto it = lookup.find(normalizeToken(pokemonName));
    if (it == lookup.end()) {
        return nullptr;
    }
    return json{{"side_index", it->second.sideIndex}, {"pokemon_index", it->second.pokemonIndex}};
}

int itemTypeToDataId(ItemType itemType) {
    if (itemType == ItemType::None) {
        return 0;
    }
    return getItemDataByName(getItemName(itemType)).id;
}

int statusTokenToIndex(const std::string& token) {
    const std::string key = normalizeToken(token);
    if (key == "burn") return static_cast<int>(StatusType::Burn);
    if (key == "freeze") return static_cast<int>(StatusType::Freeze);
    if (key == "paralysis") return static_cast<int>(StatusType::Paralysis);
    if (key == "poison") return static_cast<int>(StatusType::Poison);
    if (key == "sleep") return static_cast<int>(StatusType::Sleep);
    if (key == "flinch") return static_cast<int>(StatusType::Flinch);
    if (key == "toxicpoison") return static_cast<int>(StatusType::ToxicPoison);
    if (key == "confusion") return static_cast<int>(StatusType::Confusion);
    return 0;
}

std::string buildDescription(const json& eventEntry) {
    const std::string eventType = eventEntry.value("event", std::string("event"));
    const json details = eventEntry.value("details", json::object());

    auto asString = [&](const char* key, const std::string& fallback = "") -> std::string {
        if (details.contains(key) && details[key].is_string()) {
            return details[key].get<std::string>();
        }
        return fallback;
    };
    auto asInt = [&](const char* key, int fallback = 0) -> int {
        if (details.contains(key) && details[key].is_number_integer()) {
            return details[key].get<int>();
        }
        return fallback;
    };
    auto reasonText = [](const std::string& reason) -> std::string {
        const std::string key = normalizeToken(reason);
        if (key == "initialsendout") return "初始上场";
        if (key == "grassyterrain") return "青草场地";
        if (key == "moveeffect") return "技能附加效果";
        if (key == "onentry") return "上场时";
        if (key == "onturnend") return "回合结束";
        if (reason.empty()) return "未知来源";
        return reason;
    };

    if (eventType == "switch_in") {
        return asString("pokemon", "宝可梦") + " 上场了（" + reasonText(asString("reason")) + "）。";
    }
    if (eventType == "switch") {
        return asString("from", "当前宝可梦") + " 被换下，" + asString("to", "后备宝可梦") + " 上场。";
    }
    if (eventType == "ability_trigger") {
        return asString("pokemon", "宝可梦") + " 的特性 " + asString("ability", "未知特性") + " 触发了。";
    }
    if (eventType == "item_trigger") {
        return asString("pokemon", "宝可梦") + " 受到了道具 " + asString("item", "未知道具") + " 的效果。";
    }
    if (eventType == "status_apply") {
        std::string text = asString("pokemon", "宝可梦") + " 受到了 " + asString("status", "异常状态") + " 的效果。";
        const std::string move = asString("move");
        if (!move.empty()) {
            text += "（来源技能：" + move + "）";
        }
        return text;
    }
    if (eventType == "heal") {
        return asString("pokemon", "宝可梦") + " 受到了 " + reasonText(asString("reason")) + " 的效果，HP 回复了 " +
               std::to_string(asInt("delta")) + "。";
    }
    if (eventType == "item_blocked") {
        return asString("pokemon", "宝可梦") + " 的道具效果被抑制了。";
    }
    if (eventType == "item_replay") {
        return asString("pokemon", "宝可梦") + " 的道具效果恢复并重新生效。";
    }
    if (eventType == "stat_change") {
        const std::string pokemon = asString("pokemon", "宝可梦");
        const std::string stat = asString("stat", "能力");
        const int delta = asInt("delta", 0);
        const std::string move = asString("move");
        std::string changeText;
        if (delta >= 3) changeText = "巨幅提升";
        else if (delta == 2) changeText = "大幅提升";
        else if (delta == 1) changeText = "提升";
        else if (delta == -1) changeText = "降低";
        else if (delta == -2) changeText = "大幅降低";
        else if (delta <= -3) changeText = "巨幅降低";
        else changeText = "变化";
        std::string text = pokemon + " 的 " + stat + " " + changeText + "了！";
        if (!move.empty()) {
            text += "（来源技能：" + move + "）";
        }
        return text;
    }
    return "发生了 " + eventType + " 事件。";
}

json buildIndexedTimeline(Battle& battle) {
    const auto lookup = buildPokemonLookup(battle);
    json timeline = json::array();

    int order = 0;
    for (const auto& entry : battle.getSpecialEvents()) {
        if (!entry.is_object()) {
            continue;
        }

        json indexed;
        indexed["timeline_index"] = order++;
        indexed["turn_index"] = entry.value("turn", 0);
        indexed["description"] = buildDescription(entry);

        json indexedDetails = json::object();
        const json details = entry.value("details", json::object());
        if (details.is_object()) {
            for (auto it = details.begin(); it != details.end(); ++it) {
                indexedDetails[it.key()] = it.value();
            }

            auto addPokemonRefIfPresent = [&](const char* field) {
                if (details.contains(field) && details[field].is_string()) {
                    indexedDetails[std::string(field) + "_ref"] = pokemonRefToJson(lookup, details[field].get<std::string>());
                }
            };
            addPokemonRefIfPresent("pokemon");
            addPokemonRefIfPresent("actor");
            addPokemonRefIfPresent("target");
            addPokemonRefIfPresent("opponent");
            addPokemonRefIfPresent("from");
            addPokemonRefIfPresent("to");

            if (details.contains("ability") && details["ability"].is_string()) {
                const AbilityType ability = getAbilityTypeByNameFromData(details["ability"].get<std::string>());
                indexedDetails["ability_index"] = getAbilityData(ability).id;
            }
            if (details.contains("item") && details["item"].is_string()) {
                indexedDetails["item_index"] = getItemDataByName(details["item"].get<std::string>()).id;
            }
            if (details.contains("move") && details["move"].is_string()) {
                indexedDetails["move_index"] = getMoveDataByName(details["move"].get<std::string>()).id;
            }
            if (details.contains("status") && details["status"].is_string()) {
                indexedDetails["status_index"] = statusTokenToIndex(details["status"].get<std::string>());
            }
            if (details.contains("side") && details["side"].is_string()) {
                const std::string sideName = details["side"].get<std::string>();
                if (sideName == battle.getSideA().getName()) {
                    indexedDetails["side_index"] = 0;
                } else if (sideName == battle.getSideB().getName()) {
                    indexedDetails["side_index"] = 1;
                }
            }
        }
        indexed["details"] = indexedDetails;
        timeline.push_back(std::move(indexed));
    }
    return timeline;
}

json sideAllInfoToJson(const Side& side, int sideIndex) {
    json sideJson;
    sideJson["side"] = sideIndex;
    sideJson["name"] = side.getName();
    sideJson["active"] = side.getActiveIndex();
    sideJson["count"] = side.getPokemonCount();

    sideJson["sideEffects"] = json{
        {"reflect", side.getReflectTurns()},
        {"lightScreen", side.getLightScreenTurns()},
        {"mist", side.getMistTurns()},
        {"safeguard", side.getSafeguardTurns()},
        {"mudSport", side.getMudSportTurns()},
        {"waterSport", side.getWaterSportTurns()},
        {"spikes", side.getSpikesLayers()},
        {"toxicSpikes", side.getToxicSpikesLayers()},
        {"stealthRock", side.hasStealthRock() ? 1 : 0}
    };

    json team = json::array();
    const auto& all = side.getTeam();
    for (int i = 0; i < side.getPokemonCount(); ++i) {
        const Pokemon* pokemon = all[i];
        if (!pokemon) {
            continue;
        }

        json one;
        one["slot"] = i;
        one["speciesId"] = pokemon->getSpecies().id;
        one["hp"] = pokemon->getCurrentHP();
        one["maxHp"] = pokemon->getMaxHP();
        one["fainted"] = pokemon->isFainted();
        one["abilityId"] = getAbilityData(pokemon->getAbility()).id;
        one["itemId"] = itemTypeToDataId(pokemon->getItemType());
        one["types"] = json::array({static_cast<int>(pokemon->getType1()), static_cast<int>(pokemon->getType2())});
        one["statStages"] = json::array({
            pokemon->getStatStage(StatIndex::Attack),
            pokemon->getStatStage(StatIndex::Defense),
            pokemon->getStatStage(StatIndex::SpecialAttack),
            pokemon->getStatStage(StatIndex::SpecialDefense),
            pokemon->getStatStage(StatIndex::Speed),
            pokemon->getAccuracyStage(),
            pokemon->getEvasionStage()
        });

        json statuses = json::array();
        for (const auto& statusEntry : pokemon->getStatuses()) {
            statuses.push_back(json{
                {"id", static_cast<int>(statusEntry.first)},
                {"duration", statusEntry.second}
            });
        }
        one["inBattleStatus"] = statuses;

        json moves = json::array();
        const auto& moveList = pokemon->getMoves();
        for (std::size_t moveSlot = 0; moveSlot < moveList.size(); ++moveSlot) {
            const Move& move = moveList[moveSlot];
            moves.push_back(json{
                {"slot", static_cast<int>(moveSlot)},
                {"id", move.getData().id},
                {"pp", move.getPP()},
                {"maxPp", move.getMaxPP()}
            });
        }
        one["moves"] = moves;
        team.push_back(std::move(one));
    }

    sideJson["pokemons"] = team;
    return sideJson;
}

} // namespace

// 将Type转换为字符串
std::string BattleToJson::typeToString(Type type) {
    switch (type) {
        case Type::Normal: return "normal";
        case Type::Fire: return "fire";
        case Type::Water: return "water";
        case Type::Electric: return "electric";
        case Type::Grass: return "grass";
        case Type::Ice: return "ice";
        case Type::Fighting: return "fighting";
        case Type::Poison: return "poison";
        case Type::Ground: return "ground";
        case Type::Flying: return "flying";
        case Type::Psychic: return "psychic";
        case Type::Bug: return "bug";
        case Type::Rock: return "rock";
        case Type::Ghost: return "ghost";
        case Type::Dragon: return "dragon";
        case Type::Dark: return "dark";
        case Type::Steel: return "steel";
        case Type::Fairy: return "fairy";
        default: return "count";
    }
}

// 将ActionType转换为字符串
std::string BattleToJson::actionTypeToString(ActionType type) {
    switch (type) {
        case ActionType::Attack: return "attack";
        case ActionType::Switch: return "switch";
        case ActionType::UseItem: return "use_item";
        case ActionType::Pass: return "pass";
        default: return "pass";
    }
}

// 将Nature转换为字符串
std::string BattleToJson::natureToString(Nature nature) {
    switch (nature) {
        case Nature::Hardy: return "hardy";
        case Nature::Lonely: return "lonely";
        case Nature::Brave: return "brave";
        case Nature::Adamant: return "adamant";
        case Nature::Naughty: return "naughty";
        case Nature::Bold: return "bold";
        case Nature::Docile: return "docile";
        case Nature::Relaxed: return "relaxed";
        case Nature::Impish: return "impish";
        case Nature::Lax: return "lax";
        case Nature::Timid: return "timid";
        case Nature::Hasty: return "hasty";
        case Nature::Serious: return "serious";
        case Nature::Jolly: return "jolly";
        case Nature::Naive: return "naive";
        case Nature::Modest: return "modest";
        case Nature::Mild: return "mild";
        case Nature::Quiet: return "quiet";
        case Nature::Bashful: return "bashful";
        case Nature::Rash: return "rash";
        case Nature::Calm: return "calm";
        case Nature::Gentle: return "gentle";
        case Nature::Sassy: return "sassy";
        case Nature::Careful: return "careful";
        case Nature::Quirky: return "quirky";
        default: return "hardy";
    }
}

// 将AbilityType转换为字符串
std::string BattleToJson::abilityTypeToString(AbilityType ability) {
    switch (ability) {
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
        case AbilityType::PoisonPoint: return "poison-point";
        case AbilityType::Aftermath: return "aftermath";
        case AbilityType::Mummy: return "mummy";
        case AbilityType::RoughSkin: return "rough-skin";
        case AbilityType::FlameBody: return "flame-body";
        case AbilityType::Insomnia: return "insomnia";
        case AbilityType::VitalSpirit: return "vital-spirit";
        case AbilityType::Guts: return "guts";
        case AbilityType::HugePower: return "huge-power";
        case AbilityType::ThickFat: return "thick-fat";
        case AbilityType::MarvelScale: return "marvel-scale";
        case AbilityType::SapSipper: return "sap-sipper";
        case AbilityType::IronBarbs: return "iron-barbs";
        case AbilityType::StormDrain: return "storm-drain";
        case AbilityType::MotorDrive: return "motor-drive";
        case AbilityType::Immunity: return "immunity";
        case AbilityType::Technician: return "technician";
        case AbilityType::Filter: return "filter";
        case AbilityType::SolidRock: return "solid-rock";
        case AbilityType::Moxie: return "moxie";
        case AbilityType::InnerFocus: return "inner-focus";
        case AbilityType::Regenerator: return "regenerator";
        case AbilityType::NaturalCure: return "natural-cure";
        case AbilityType::MagicGuard: return "magic-guard";
        case AbilityType::Unaware: return "unaware";
        case AbilityType::Prankster: return "prankster";
        case AbilityType::ClearBody: return "clear-body";
        case AbilityType::Defiant: return "defiant";
        case AbilityType::Competitive: return "competitive";
        case AbilityType::WhiteSmoke: return "white-smoke";
        case AbilityType::MirrorArmor: return "mirror-armor";
        case AbilityType::HyperCutter: return "hyper-cutter";
        case AbilityType::KeenEye: return "keen-eye";
        case AbilityType::Drizzle: return "drizzle";
        case AbilityType::Drought: return "drought";
        case AbilityType::SandStream: return "sand-stream";
        case AbilityType::SnowWarning: return "snow-warning";
        case AbilityType::CloudNine: return "cloud-nine";
        case AbilityType::GrassySurge: return "grassy-surge";
        case AbilityType::ElectricSurge: return "electric-surge";
        case AbilityType::PsychicSurge: return "psychic-surge";
        case AbilityType::MistySurge: return "misty-surge";
        case AbilityType::HadronEngine: return "hadron-engine";
        case AbilityType::Protean: return "protean";
        case AbilityType::Libero: return "libero";
        case AbilityType::Adaptability: return "adaptability";
        case AbilityType::SheerForce: return "sheer-force";
        case AbilityType::Infiltrator: return "infiltrator";
        case AbilityType::BeadsOfRuin: return "beads-of-ruin";
        case AbilityType::SwordOfRuin: return "sword-of-ruin";
        case AbilityType::TabletsOfRuin: return "tablets-of-ruin";
        case AbilityType::VesselOfRuin: return "vessel-of-ruin";
        case AbilityType::Unnerve: return "unnerve";
        case AbilityType::Sharpness: return "sharpness";
        case AbilityType::EarthEater: return "earth-eater";
        case AbilityType::PurifyingSalt: return "purifying-salt";
        case AbilityType::WellBakedBody: return "well-baked-body";
        case AbilityType::WindRider: return "wind-rider";
        case AbilityType::ToxicDebris: return "toxic-debris";
        case AbilityType::LingeringAroma: return "lingering-aroma";
        case AbilityType::ArmorTail: return "armor-tail";
        case AbilityType::GoodAsGold: return "good-as-gold";
        case AbilityType::Stakeout: return "stakeout";
        case AbilityType::CudChew: return "cud-chew";
        case AbilityType::MoldBreaker: return "mold-breaker";
        case AbilityType::Protosynthesis: return "protosynthesis";
        case AbilityType::QuarkDrive: return "quark-drive";
        case AbilityType::SupremeOverlord: return "supreme-overlord";
        default: return "none";
    }
}

// 将ItemType转换为字符串
std::string BattleToJson::itemTypeToString(ItemType item) {
    if (item == ItemType::None) {
        return "none";
    }
    return getItemName(item);
}

std::string BattleToJson::statusTypeToString(StatusType status) {
    switch (status) {
        case StatusType::Burn: return "burn";
        case StatusType::Freeze: return "freeze";
        case StatusType::Paralysis: return "paralysis";
        case StatusType::Poison: return "poison";
        case StatusType::Sleep: return "sleep";
        case StatusType::Flinch: return "flinch";
        case StatusType::ToxicPoison: return "toxic_poison";
        case StatusType::Confusion: return "confusion";
        default: return "none";
    }
}

std::string BattleToJson::weatherTypeToString(WeatherType weather) {
    switch (weather) {
        case WeatherType::Rain: return "rain";
        case WeatherType::Sun: return "sun";
        case WeatherType::Sandstorm: return "sandstorm";
        case WeatherType::Hail: return "hail";
        case WeatherType::Snow: return "snow";
        default: return "clear";
    }
}

std::string BattleToJson::fieldTypeToString(FieldType field) {
    switch (field) {
        case FieldType::Psychic: return "psychic";
        case FieldType::Electric: return "electric";
        case FieldType::Grassy: return "grassy";
        case FieldType::Misty: return "misty";
        case FieldType::TrickRoom: return "trick_room";
        default: return "none";
    }
}

// 将BattleAction转换为Json
json BattleToJson::actionToJson(const BattleAction& action) {
    json jsonAction;
    jsonAction["type"] = actionTypeToString(action.type);
    jsonAction["priority"] = action.priority;
    
    if (action.actor) {
        jsonAction["actor"] = action.actor->getName();
    }
    
    if (action.target) {
        jsonAction["target"] = action.target->getName();
    }
    
    if (action.type == ActionType::Attack) {
        jsonAction["move"] = action.move.getName();
    } else if (action.type == ActionType::Switch) {
        jsonAction["switch_index"] = action.switchIndex;
    } else if (action.type == ActionType::UseItem) {
        jsonAction["item"] = itemTypeToString(action.item);
    }
    
    return jsonAction;
}

// 将Pokemon转换为Json
json BattleToJson::pokemonToJson(const Pokemon* pokemon) {
    if (!pokemon) {
        return json();
    }
    
    json jsonPokemon;
    jsonPokemon["name"] = pokemon->getName();
    jsonPokemon["level"] = pokemon->getLevel();
    jsonPokemon["current_hp"] = pokemon->getCurrentHP();
    jsonPokemon["max_hp"] = pokemon->getMaxHP();
    jsonPokemon["is_fainted"] = pokemon->isFainted();
    
    // 类型
    jsonPokemon["type1"] = typeToString(pokemon->getType1());
    jsonPokemon["type2"] = typeToString(pokemon->getType2());
    
    // 特性
    jsonPokemon["ability"] = abilityTypeToString(pokemon->getAbility());
    
    // 性格
    jsonPokemon["nature"] = natureToString(pokemon->getNature());
    
    // 物品
    jsonPokemon["item"] = itemTypeToString(pokemon->getItemType());
    
    // 技能
    json movesArray = json::array();
    for (const auto& move : pokemon->getMoves()) {
        json moveJson;
        moveJson["name"] = move.getName();
        moveJson["type"] = typeToString(move.getType());
        moveJson["power"] = move.getPower();
        moveJson["accuracy"] = move.getAccuracy();
        moveJson["pp"] = move.getPP();
        moveJson["max_pp"] = move.getMaxPP();
        movesArray.push_back(moveJson);
    }
    jsonPokemon["moves"] = movesArray;

    json statStages;
    statStages["attack"] = pokemon->getStatStage(StatIndex::Attack);
    statStages["defense"] = pokemon->getStatStage(StatIndex::Defense);
    statStages["special_attack"] = pokemon->getStatStage(StatIndex::SpecialAttack);
    statStages["special_defense"] = pokemon->getStatStage(StatIndex::SpecialDefense);
    statStages["speed"] = pokemon->getStatStage(StatIndex::Speed);
    statStages["accuracy"] = pokemon->getAccuracyStage();
    statStages["evasion"] = pokemon->getEvasionStage();
    jsonPokemon["stat_stages"] = statStages;

    json statuses = json::array();
    for (const auto& statusEntry : pokemon->getStatuses()) {
        json one;
        one["type"] = statusTypeToString(statusEntry.first);
        one["duration"] = statusEntry.second;
        statuses.push_back(one);
    }
    jsonPokemon["statuses"] = statuses;

    json stats;
    stats["attack"] = pokemon->getAttack();
    stats["defense"] = pokemon->getDefense();
    stats["special_attack"] = pokemon->getSpecialAttack();
    stats["special_defense"] = pokemon->getSpecialDefense();
    stats["speed"] = pokemon->getSpeed();
    jsonPokemon["stats"] = stats;
    
    return jsonPokemon;
}

// 将Side转换为Json
json BattleToJson::sideToJson(const Side& side) {
    json jsonSide;
    jsonSide["name"] = side.getName();
    jsonSide["active_index"] = side.getActiveIndex();
    jsonSide["pokemon_count"] = side.getPokemonCount();
    
    // 当前活跃的宝可梦
    if (side.getActivePokemon()) {
        jsonSide["active_pokemon"] = pokemonToJson(side.getActivePokemon());
    }

    json team = json::array();
    const auto& all = side.getTeam();
    for (int i = 0; i < side.getPokemonCount(); ++i) {
        team.push_back(pokemonToJson(all[i]));
    }
    jsonSide["team"] = team;
    
    return jsonSide;
}

json BattleToJson::battleAllInfoToJson(Battle& battle) {
    json timeline = buildIndexedTimeline(battle);
    json descriptions = json::array();
    for (const auto& entry : timeline) {
        if (entry.contains("description") && entry["description"].is_string()) {
            descriptions.push_back(entry["description"]);
        }
    }

    json battleState;
    battleState["turn"] = battle.getTurnNumber();
    battleState["sides"] = json::array({
        sideAllInfoToJson(battle.getSideA(), 0),
        sideAllInfoToJson(battle.getSideB(), 1),
    });
    battleState["field"] = json{
        {"type", static_cast<int>(battle.getField().type)},
        {"duration", battle.getField().duration}
    };
    battleState["weather"] = json{
        {"type", static_cast<int>(battle.getWeather().type)},
        {"duration", battle.getWeather().duration}
    };

    json allInfo;
    allInfo["turn"] = battle.getTurnNumber();
    allInfo["battle"] = std::move(battleState);
    allInfo["events"] = timeline;
    allInfo["descriptions"] = descriptions;
    return allInfo;
}

// 将Battle转换为Json
json BattleToJson::battleToJson(Battle& battle) {
    json jsonBattle;
    jsonBattle["turn"] = battle.getTurnNumber();
    
    // 双方
    jsonBattle["side_a"] = sideToJson(battle.getSideA());
    jsonBattle["side_b"] = sideToJson(battle.getSideB());

    json fieldJson;
    fieldJson["type"] = fieldTypeToString(battle.getField().type);
    fieldJson["duration"] = battle.getField().duration;
    jsonBattle["field"] = fieldJson;

    json weatherJson;
    weatherJson["type"] = weatherTypeToString(battle.getWeather().type);
    weatherJson["duration"] = battle.getWeather().duration;
    jsonBattle["weather"] = weatherJson;
    jsonBattle["battle_all_info"] = battleAllInfoToJson(battle);
    
    return jsonBattle;
}

// 将Json数据写入到cache文件中
void BattleToJson::writeToCache(const json& data, const std::string& filename) {
    if (filename.rfind("output_", 0) != 0) {
        return;
    }
    // 确保cache/output目录存在
    std::string cacheDir = "cache/output";
    std::filesystem::create_directories(cacheDir);
    
    // 构建完整的文件路径
    std::string filePath = cacheDir + "/" + filename;
    
    // 打开文件
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return;
    }
    
    nlohmann::ordered_json orderedData;
    if (data.is_object()) {
        if (data.contains("descriptions")) orderedData["descriptions"] = data["descriptions"];
        if (data.contains("turn")) orderedData["turn"] = data["turn"];
        if (data.contains("battle")) orderedData["battle"] = data["battle"];
        if (data.contains("events")) orderedData["events"] = data["events"];
        for (auto it = data.begin(); it != data.end(); ++it) {
            if (orderedData.contains(it.key())) {
                continue;
            }
            orderedData[it.key()] = it.value();
        }
    } else {
        orderedData = data;
    }

    // 写入Json数据
    file << orderedData.dump(2);
    file.close();
}
