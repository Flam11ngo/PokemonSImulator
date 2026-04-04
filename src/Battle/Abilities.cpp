#include "Abilities.h"
#include "Pokemon.h" // For Pokemon class
#include <iostream>

Ability getAbility(AbilityType type) {
    Ability ability;
    switch (type) {
        case AbilityType::Intimidate:
            ability.name = "Intimidate";
            ability.type = AbilityType::Intimidate;
            ability.effects[Trigger::OnEntry] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!opponent || !self) return;
                std::cout << self->getName() << "'s Intimidate lowered the opponent's Attack!" << std::endl;
                // 降低对手攻击
                // 这里需要实现降低对手攻击的逻辑
                // 由于Pokemon类没有直接修改攻击的方法，这里暂时使用占位符
            };
            break;
        case AbilityType::Overgrow:
            ability.name = "Overgrow";
            ability.type = AbilityType::Overgrow;
            ability.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self) return;
                // 检查HP是否低于33%
                if (self->getCurrentHP() < self->getMaxHP() / 3) {
                    std::cout << self->getName() << "'s Overgrow boosted the power of Grass-type moves!" << std::endl;
                    // 草系技能威力提升50%
                    // 这里需要在伤害计算中实现
                }
            };
            break;
        case AbilityType::Blaze:
            ability.name = "Blaze";
            ability.type = AbilityType::Blaze;
            ability.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self) return;
                // 检查HP是否低于33%
                if (self->getCurrentHP() < self->getMaxHP() / 3) {
                    std::cout << self->getName() << "'s Blaze boosted the power of Fire-type moves!" << std::endl;
                    // 火系技能威力提升50%
                    // 这里需要在伤害计算中实现
                }
            };
            break;
        case AbilityType::Torrent:
            ability.name = "Torrent";
            ability.type = AbilityType::Torrent;
            ability.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self) return;
                // 检查HP是否低于33%
                if (self->getCurrentHP() < self->getMaxHP() / 3) {
                    std::cout << self->getName() << "'s Torrent boosted the power of Water-type moves!" << std::endl;
                    // 水系技能威力提升50%
                    // 这里需要在伤害计算中实现
                }
            };
            break;
        case AbilityType::Multiscale:
            ability.name = "Multiscale";
            ability.type = AbilityType::Multiscale;
            ability.effects[Trigger::OnDamage] = [](Pokemon* self, Pokemon* opponent, void* context) {
                if (!self) return;
                // 检查是否满血
                if (self->getCurrentHP() == self->getMaxHP()) {
                    std::cout << self->getName() << "'s Multiscale reduced the damage taken!" << std::endl;
                    // 受到的伤害减少50%
                    // 这里需要在伤害计算中实现
                }
            };
            break;
        default:
            ability.name = "None";
            ability.type = AbilityType::None;
            break;
    }
    return ability;
}

std::vector<Ability> getAbilitiesForPokemon(AbilityType type) {
    // For now, each Pokemon has one ability
    std::vector<Ability> abilities;
    abilities.push_back(getAbility(type));
    return abilities;
}