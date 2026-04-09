#pragma once

#include "OnCondition.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Pokemon;

enum class AbilityType {
    None,
    Intimidate,
    Overgrow,
    Blaze,
    Torrent,
    Multiscale,
    Levitate,
    WaterAbsorb,
    VoltAbsorb,
    FlashFire,
    Static,
    // Add more
    Count
};

struct StatModifier {
    enum Stat { Attack, Defense, SpAttack, SpDefense, Speed, Accuracy, Evasion };
    Stat stat;
    float multiplier;  // 倍率
    int flatBonus;     // 固定加成
};


struct TypeImmunity {
    int typeId;  // 属性ID
    bool healInstead;  // 是否回复HP
    int healPercent;   // 回复百分比
};

struct StatusImmunity {
    int statusId;  // 状态ID
};

struct DamageModifier {
    float multiplier;
    bool onDealDamage;   // true: 造成伤害时, false: 受到伤害时
};

struct AbilityData {
    int id;
    std::string name;
    std::string apiName;
    std::string description;
    AbilityType type;
};

class Ability {
private:
    AbilityData data;

public:
    // 1. 基础效果（多个触发时机）
    std::unordered_map<Trigger, std::function<void(Pokemon* self, Pokemon* opponent, void* context)>> effects;
    
    // 2. 数值修改
    std::vector<StatModifier> statModifiers;
    
    // 3. 免疫效果
    std::vector<TypeImmunity> typeImmunities;
    std::vector<StatusImmunity> statusImmunities;
    
    // 4. 伤害修改
    DamageModifier damageModifier;

    Ability() = default;
    explicit Ability(const AbilityData& abilityData) : data(abilityData) {}

    const AbilityData& getData() const { return data; }
    const std::string& getName() const { return data.name; }
    AbilityType getType() const { return data.type; }
    void setData(const AbilityData& abilityData) { data = abilityData; }
    
    // 辅助方法
    bool hasTrigger(Trigger trigger) const {
        return effects.find(trigger) != effects.end();
    }
    
    void executeTrigger(Trigger trigger, Pokemon* self, Pokemon* opponent, void* context = nullptr) const {
        auto it = effects.find(trigger);
        if (it != effects.end() && it->second) {
            it->second(self, opponent, context);
        }
    }
};

// Get ability struct by type
Ability getAbility(AbilityType type);

// Convert between ability enum and canonical display name.
std::string getAbilityName(AbilityType type);
AbilityType getAbilityTypeByName(const std::string& name);

// Ability data lookup and conversion helpers.
AbilityData getAbilityDataById(int id);
AbilityData getAbilityDataByName(const std::string& name);
AbilityData getAbilityData(AbilityType type);
AbilityType getAbilityTypeById(int id);
AbilityType getAbilityTypeByNameFromData(const std::string& name);

// Prefetch referenced ability entries and persist to data/abilities.json.
bool prefetchAbilitiesFromPokeAPI(bool refreshExisting = false);

// Get all abilities for a Pokemon (in case multiple)
std::vector<Ability> getAbilitiesForPokemon(AbilityType type);
