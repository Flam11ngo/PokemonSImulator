#include "Battle/BuildFromJson.h"
#include "Battle/Types.h"
#include "Battle/Natures.h"
#include "Battle/Abilities.h"
#include "Battle/Items.h"
#include "Battle/Status.h"
#include "Battle/Moves.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// 辅助函数：从字符串转换为Type枚举
Type stringToType(const std::string& typeStr) {
    if (typeStr == "normal") return Type::Normal;
    if (typeStr == "fire") return Type::Fire;
    if (typeStr == "water") return Type::Water;
    if (typeStr == "electric") return Type::Electric;
    if (typeStr == "grass") return Type::Grass;
    if (typeStr == "ice") return Type::Ice;
    if (typeStr == "fighting") return Type::Fighting;
    if (typeStr == "poison") return Type::Poison;
    if (typeStr == "ground") return Type::Ground;
    if (typeStr == "flying") return Type::Flying;
    if (typeStr == "psychic") return Type::Psychic;
    if (typeStr == "bug") return Type::Bug;
    if (typeStr == "rock") return Type::Rock;
    if (typeStr == "ghost") return Type::Ghost;
    if (typeStr == "dragon") return Type::Dragon;
    if (typeStr == "dark") return Type::Dark;
    if (typeStr == "steel") return Type::Steel;
    if (typeStr == "fairy") return Type::Fairy;
    return Type::Count;
}

// 辅助函数：从字符串转换为Nature枚举
Nature stringToNature(const std::string& natureStr) {
    if (natureStr == "hardy") return Nature::Hardy;
    if (natureStr == "lonely") return Nature::Lonely;
    if (natureStr == "brave") return Nature::Brave;
    if (natureStr == "adamant") return Nature::Adamant;
    if (natureStr == "naughty") return Nature::Naughty;
    if (natureStr == "bold") return Nature::Bold;
    if (natureStr == "docile") return Nature::Docile;
    if (natureStr == "relaxed") return Nature::Relaxed;
    if (natureStr == "impish") return Nature::Impish;
    if (natureStr == "lax") return Nature::Lax;
    if (natureStr == "timid") return Nature::Timid;
    if (natureStr == "hasty") return Nature::Hasty;
    if (natureStr == "serious") return Nature::Serious;
    if (natureStr == "jolly") return Nature::Jolly;
    if (natureStr == "naive") return Nature::Naive;
    if (natureStr == "modest") return Nature::Modest;
    if (natureStr == "mild") return Nature::Mild;
    if (natureStr == "quiet") return Nature::Quiet;
    if (natureStr == "bashful") return Nature::Bashful;
    if (natureStr == "rash") return Nature::Rash;
    if (natureStr == "calm") return Nature::Calm;
    if (natureStr == "gentle") return Nature::Gentle;
    if (natureStr == "sassy") return Nature::Sassy;
    if (natureStr == "careful") return Nature::Careful;
    if (natureStr == "quirky") return Nature::Quirky;
    return Nature::Hardy;
}

// 辅助函数：从字符串转换为AbilityType枚举
AbilityType stringToAbility(const std::string& abilityStr) {
    if (abilityStr == "blaze") return AbilityType::Blaze;
    if (abilityStr == "torrent") return AbilityType::Torrent;
    if (abilityStr == "overgrow") return AbilityType::Overgrow;
    if (abilityStr == "intimidate") return AbilityType::Intimidate;
    return AbilityType::None;
}

// 辅助函数：从字符串转换为ItemType枚举
ItemType stringToItem(const std::string& itemStr) {
    if (itemStr == "none") return ItemType::None;
    // 添加更多物品类型
    return ItemType::None;
}

// 从Json创建Species对象
Species BuildFromJson::buildSpecies(const json& jsonData) {
    Species species;
    
    // 基本信息
    species.id = jsonData.value("id", 0);
    species.name = jsonData.value("name", "");
    species.type1 = stringToType(jsonData.value("type1", "normal"));
    species.type2 = stringToType(jsonData.value("type2", "count"));
    
    // 基础属性
    if (jsonData.contains("baseStats")) {
        const auto& stats = jsonData["baseStats"];
        for (int i = 0; i < 6 && i < stats.size(); i++) {
            species.baseStats[i] = stats[i].get<int>();
        }
    }
    
    // 蛋群
    if (jsonData.contains("eggGroups")) {
        for (const auto& eggGroup : jsonData["eggGroups"]) {
            // 简化处理，实际需要更多蛋群类型
            species.eggGroups.push_back(EggGroup::Monster);
        }
    }
    
    // 可学习技能
    if (jsonData.contains("learnableMoves")) {
        for (const auto& moveData : jsonData["learnableMoves"]) {
            std::string moveName = moveData.value("name", "");
            Type moveType = stringToType(moveData.value("type", "normal"));
            Category moveCategory = Category::Physical; // 默认值
            int movePower = moveData.value("power", 0);
            int moveAccuracy = moveData.value("accuracy", 100);
            int movePP = moveData.value("pp", 10);
            
            Move move(moveName, moveType, moveCategory, movePower, moveAccuracy, movePP);
            species.learnableMoves.push_back(move);
        }
    }
    
    // 性别比例
    species.maleRatio = jsonData.value("maleRatio", 0.5f);
    
    // 进化信息
    species.nextEvolutionID = jsonData.value("nextEvolutionID", -1);
    species.evolutionLevel = jsonData.value("evolutionLevel", 0);
    
    // 特性
    if (jsonData.contains("abilities")) {
        for (const auto& abilityStr : jsonData["abilities"]) {
            species.abilities.push_back(stringToAbility(abilityStr.get<std::string>()));
        }
    }
    
    return species;
}

// 从Json创建Pokemon对象
Pokemon BuildFromJson::buildPokemon(const json& jsonData, const Species* species) {
    // 初始化IVs和EVs
    std::array<int, static_cast<int>(StatIndex::Count)> ivs;
    std::array<int, static_cast<int>(StatIndex::Count)> evs;
    
    // 默认值
    ivs.fill(31); // 满IV
    evs.fill(0);  // 0 EV
    
    // 从Json读取IVs
    if (jsonData.contains("ivs")) {
        const auto& ivData = jsonData["ivs"];
        if (ivData.contains("hp")) ivs[static_cast<int>(StatIndex::HP)] = ivData["hp"].get<int>();
        if (ivData.contains("attack")) ivs[static_cast<int>(StatIndex::Attack)] = ivData["attack"].get<int>();
        if (ivData.contains("defense")) ivs[static_cast<int>(StatIndex::Defense)] = ivData["defense"].get<int>();
        if (ivData.contains("specialAttack")) ivs[static_cast<int>(StatIndex::SpecialAttack)] = ivData["specialAttack"].get<int>();
        if (ivData.contains("specialDefense")) ivs[static_cast<int>(StatIndex::SpecialDefense)] = ivData["specialDefense"].get<int>();
        if (ivData.contains("speed")) ivs[static_cast<int>(StatIndex::Speed)] = ivData["speed"].get<int>();
    }
    
    // 从Json读取EVs
    if (jsonData.contains("evs")) {
        const auto& evData = jsonData["evs"];
        if (evData.contains("hp")) evs[static_cast<int>(StatIndex::HP)] = evData["hp"].get<int>();
        if (evData.contains("attack")) evs[static_cast<int>(StatIndex::Attack)] = evData["attack"].get<int>();
        if (evData.contains("defense")) evs[static_cast<int>(StatIndex::Defense)] = evData["defense"].get<int>();
        if (evData.contains("specialAttack")) evs[static_cast<int>(StatIndex::SpecialAttack)] = evData["specialAttack"].get<int>();
        if (evData.contains("specialDefense")) evs[static_cast<int>(StatIndex::SpecialDefense)] = evData["specialDefense"].get<int>();
        if (evData.contains("speed")) evs[static_cast<int>(StatIndex::Speed)] = evData["speed"].get<int>();
    }
    
    // 读取其他属性
    Nature nature = stringToNature(jsonData.value("nature", "hardy"));
    AbilityType ability = stringToAbility(jsonData.value("ability", "none"));
    int level = jsonData.value("level", 50);
    
    // 创建Pokemon对象
    Pokemon pokemon(species, nature, ability, level, ivs, evs);
    
    // 设置持有物品
    if (jsonData.contains("item")) {
        pokemon.setItemType(stringToItem(jsonData["item"].get<std::string>()));
    }
    
    // 设置技能
    if (jsonData.contains("moves")) {
        for (const auto& moveData : jsonData["moves"]) {
            std::string moveName = moveData.value("name", "");
            Type moveType = stringToType(moveData.value("type", "normal"));
            Category moveCategory = Category::Physical; // 默认值
            int movePower = moveData.value("power", 0);
            int moveAccuracy = moveData.value("accuracy", 100);
            int movePP = moveData.value("pp", 10);
            
            Move move(moveName, moveType, moveCategory, movePower, moveAccuracy, movePP);
            pokemon.addMove(move);
        }
    }
    
    return pokemon;
}

// 从Json文件加载宝可梦数据
Pokemon BuildFromJson::loadPokemonFromFile(const std::string& filePath) {
    // 尝试使用不同的路径
    std::string paths[] = {
        filePath,
        "../" + filePath,
        "../../" + filePath
    };
    
    std::ifstream file;
    bool fileOpened = false;
    
    for (const auto& path : paths) {
        file.open(path);
        if (file.is_open()) {
            fileOpened = true;
            break;
        }
        file.close();
    }
    
    if (!fileOpened) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        // 返回一个默认的宝可梦
        Species* defaultSpecies = new Species();
        Pokemon pokemon(defaultSpecies, Nature::Hardy, AbilityType::None, 50, {}, {});
        return pokemon;
    }
    
    json jsonData;
    file >> jsonData;
    file.close();
    
    // 构建Species
    Species* species = new Species(buildSpecies(jsonData));
    
    // 构建Pokemon
    Pokemon pokemon = buildPokemon(jsonData, species);
    return pokemon;
}

// 从Json字符串加载宝可梦数据
Pokemon BuildFromJson::loadPokemonFromString(const std::string& jsonString) {
    json jsonData = json::parse(jsonString);
    
    // 构建Species
    Species* species = new Species(buildSpecies(jsonData));
    
    // 构建Pokemon
    Pokemon pokemon = buildPokemon(jsonData, species);
    return pokemon;
}
