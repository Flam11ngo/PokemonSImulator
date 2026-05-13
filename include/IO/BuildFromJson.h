#pragma once

#include "battle/Pokemon.h"
#include "battle/Species.h"
#include <nlohmann/json.hpp>
#include <map>
#include <string>
#include <vector>

using json = nlohmann::json;

// 从species.json加载所有种族信息，返回 id -> Species 的映射
std::map<int, Species> loadSpeciesFromFile();

// 从abilities.json加载所有特性信息，返回 id -> Ability 的映射
std::map<int, Ability> loadAbilitiesFromFile();

// 从items.json加载所有道具信息，返回 id -> Item 的映射
std::map<int, Item> loadItemsFromFile();

// 从moves.json加载所有技能信息，返回 id -> Move 的映射
std::map<int, Move> loadMovesFromFile();

class BuildFromJson {
public:
    // 从Json创建Species对象
    static Species buildSpecies(const json& jsonData);
    
    // 从Json创建Pokemon对象
    static Pokemon buildPokemon(const json& jsonData, const Species& species);
    
    // 从Json文件加载宝可梦数据
    static Pokemon loadPokemonFromFile(const std::string& filePath);

    // 从Json文件加载队伍数据（支持 {"pokemon": [...]} 格式和单只宝可梦格式）
    static std::vector<Pokemon> loadPokemonTeamFromFile(const std::string& filePath);

    // 从Json字符串加载宝可梦数据
    static Pokemon loadPokemonFromString(const std::string& jsonString);
};
