#pragma once

#include "battle/Abilities.h"
#include "battle/Items.h"
#include <functional>
#include <string>
#include <unordered_map>

class Battle;
class Pokemon;
class Move;

using MoveRuleHandler = std::function<bool(BattleContext&, Pokemon*, Pokemon*, const Move&)>;

// Ability builder: receives pre-populated Ability + immunity helpers
using AddTypeImmunity   = std::function<void(Type, bool, int)>;
using AddStatusImmunity = std::function<void(StatusType)>;
using AbilityBuilder    = std::function<void(Ability&, AddTypeImmunity, AddStatusImmunity)>;

// Item builder: receives pre-constructed Item and configures it
using ItemBuilder = std::function<void(Item&)>;

class GameRegistry {
public:
    GameRegistry() = default;

    void init();

    const Ability& getAbility(AbilityType type) const;
    const Item& getItem(ItemType type) const;
    const MoveRuleHandler* getMoveRule(const std::string& normalizedMoveName) const;

    bool applyMoveRule(const std::string& name, BattleContext& ctx,
                       Pokemon* attacker, Pokemon* defender, const Move& move) const;

    bool hasAbility(AbilityType type) const;
    bool hasItem(ItemType type) const;
    bool isInitialized() const { return initialized; }

    void registerMoveRule(const std::string& normalizedMoveName, MoveRuleHandler handler);
    void registerAbilityBuilder(AbilityType type, AbilityBuilder builder);
    void registerItemBuilder(ItemType type, ItemBuilder builder);

    static GameRegistry& instance();

private:
    bool initialized = false;

private:
    void initAbilities();
    void initItems();
    void initMoveRules();

    std::unordered_map<AbilityType, Ability> abilities;
    std::unordered_map<ItemType, Item> items;
    std::unordered_map<std::string, MoveRuleHandler> moveRules;
    std::unordered_map<AbilityType, AbilityBuilder> abilityBuilders;
    std::unordered_map<ItemType, ItemBuilder> itemBuilders;
};
