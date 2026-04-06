#include "Battle/BattleToJson.h"
#include "Battle/Types.h"
#include "Battle/Natures.h"
#include "Battle/Abilities.h"
#include "Battle/Items.h"
#include "Battle/Status.h"
#include "Battle/Weather.h"
#include "Battle/Field.h"
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>

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
        default: return "none";
    }
}

// 将ItemType转换为字符串
std::string BattleToJson::itemTypeToString(ItemType item) {
    switch (item) {
        case ItemType::None: return "none";
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
    
    return jsonPokemon;
}

// 将Side转换为Json
json BattleToJson::sideToJson(const Side& side) {
    json jsonSide;
    jsonSide["name"] = side.getName();
    
    // 当前活跃的宝可梦
    if (side.getActivePokemon()) {
        jsonSide["active_pokemon"] = side.getActivePokemon()->getName();
    }
    
    return jsonSide;
}

// 将Battle转换为Json
json BattleToJson::battleToJson(Battle& battle) {
    json jsonBattle;
    jsonBattle["turn"] = battle.getTurnNumber();
    
    // 双方
    jsonBattle["side_a"] = sideToJson(battle.getSideA());
    jsonBattle["side_b"] = sideToJson(battle.getSideB());
    
    return jsonBattle;
}

// 将Json数据写入到cache文件中
void BattleToJson::writeToCache(const json& data, const std::string& filename) {
    // 确保cache目录存在
    std::string cacheDir = "cache";
    std::filesystem::create_directories(cacheDir);
    
    // 构建完整的文件路径
    std::string filePath = cacheDir + "/" + filename;
    
    // 打开文件
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return;
    }
    
    // 写入Json数据
    file << data.dump(2);
    file.close();
}