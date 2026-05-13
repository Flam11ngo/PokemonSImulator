#include "battle/GameRegistry.h"
#include "battle/Moves.h"

#include <cctype>

GameRegistry& GameRegistry::instance() {
    static GameRegistry registry;
    if (!registry.initialized) {
        registry.init();
    }
    return registry;
}

void GameRegistry::init() {
    if (initialized) return;
    initialized = true;  // Set early to prevent recursive init from getAbility() during builder execution
    initAbilities();
    initItems();
    initMoveRules();
}

void GameRegistry::initAbilities() {
    ::initializeCoreAbilities(*this);

    for (int i = 0; i < static_cast<int>(AbilityType::Count); ++i) {
        AbilityType type = static_cast<AbilityType>(i);
        auto it = abilityBuilders.find(type);
        if (it != abilityBuilders.end()) {
            Ability ability;
            AbilityData data = getAbilityData(type);
            if (data.type == AbilityType::None && type != AbilityType::None) {
                data.type = type;
            }
            if (data.name.empty() || data.name == "None") {
                data.name = getAbilityName(type);
            }
            ability = Ability(data);
            ability.damageModifier = {1.0f, true};

            AddTypeImmunity addType = [&ability](Type moveType, bool healInstead, int healPercent) {
                ability.typeImmunities.push_back({static_cast<int>(moveType), healInstead, healPercent});
            };
            AddStatusImmunity addStatus = [&ability](StatusType status) {
                ability.statusImmunities.push_back({static_cast<int>(status)});
            };

            it->second(ability, addType, addStatus);
            abilities.emplace(type, std::move(ability));
        } else {
            // Fallback: use old switch-based getAbility() for not-yet-migrated types
            abilities.emplace(type, ::getAbility(type));
        }
    }
}

void GameRegistry::initItems() {
    ::initializeCoreItems(*this);

    for (int i = 0; i < static_cast<int>(ItemType::Count); ++i) {
        ItemType type = static_cast<ItemType>(i);
        auto it = itemBuilders.find(type);
        if (it != itemBuilders.end()) {
            Item item(type, getItemName(type));
            it->second(item);
            items.emplace(type, std::move(item));
        } else {
            // Fallback: use old switch-based getItem() for not-yet-migrated types
            items.emplace(type, ::getItem(type));
        }
    }
}

void GameRegistry::initMoveRules() {
    ::initializeCoreMoveRules(*this);
}

const Ability& GameRegistry::getAbility(AbilityType type) const {
    static Ability fallback;
    auto it = abilities.find(type);
    if (it != abilities.end()) {
        return it->second;
    }
    return fallback;
}

const Item& GameRegistry::getItem(ItemType type) const {
    static Item fallback;
    auto it = items.find(type);
    if (it != items.end()) {
        return it->second;
    }
    return fallback;
}

const MoveRuleHandler* GameRegistry::getMoveRule(const std::string& name) const {
    auto it = moveRules.find(name);
    if (it != moveRules.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GameRegistry::applyMoveRule(const std::string& name, BattleContext& ctx,
                                 Pokemon* attacker, Pokemon* defender, const Move& move) const {
    std::string normalized;
    normalized.reserve(name.size());
    for (char ch : name) {
        if (ch == ' ' || ch == '-' || ch == '\'' || ch == '_') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    const MoveRuleHandler* handler = getMoveRule(normalized);
    if (!handler || !*handler) {
        return false;
    }
    return (*handler)(ctx, attacker, defender, move);
}

void GameRegistry::registerMoveRule(const std::string& name, MoveRuleHandler handler) {
    std::string normalized;
    normalized.reserve(name.size());
    for (char ch : name) {
        if (ch == ' ' || ch == '-' || ch == '\'' || ch == '_') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    moveRules[normalized] = std::move(handler);
}

void GameRegistry::registerAbilityBuilder(AbilityType type, AbilityBuilder builder) {
    abilityBuilders[type] = std::move(builder);
}

void GameRegistry::registerItemBuilder(ItemType type, ItemBuilder builder) {
    itemBuilders[type] = std::move(builder);
}

bool GameRegistry::hasAbility(AbilityType type) const {
    return abilities.find(type) != abilities.end();
}

bool GameRegistry::hasItem(ItemType type) const {
    return items.find(type) != items.end();
}
