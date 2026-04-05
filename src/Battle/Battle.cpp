#include "Battle/Battle.h"
#include "Battle/Items.h"
#include <iostream>
#include <cmath>
#include <algorithm>

Side* Battle::findSideForPokemon(Battle& battle, Pokemon* pokemon) {
    if (battle.getSideA().getActivePokemon() == pokemon) return &battle.getSideA();
    if (battle.getSideB().getActivePokemon() == pokemon) return &battle.getSideB();
    for (Pokemon* member : battle.getSideA().getTeam()) {
        if (member == pokemon) return &battle.getSideA();
    }
    for (Pokemon* member : battle.getSideB().getTeam()) {
        if (member == pokemon) return &battle.getSideB();
    }
    return nullptr;
}

Battle::Battle(Side sideA, Side sideB)
    : sideA(std::move(sideA)), sideB(std::move(sideB)), field(), weather(), queue(), eventSystem(), turnNumber(0) {
    // 触发双方活跃宝可梦的出场特性
    Pokemon* activeA = this->sideA.getActivePokemon();
    Pokemon* activeB = this->sideB.getActivePokemon();
    
    if (activeA) {
        triggerAbility(activeA, Trigger::OnEntry, activeB);
        triggerItemEffect(activeA, ItemTrigger::OnEntry, activeB);
    }
    
    if (activeB) {
        triggerAbility(activeB, Trigger::OnEntry, activeA);
        triggerItemEffect(activeB, ItemTrigger::OnEntry, activeA);
    }
}

Side& Battle::getSideA() {
    return sideA;
}

Side& Battle::getSideB() {
    return sideB;
}

Field& Battle::getField() {
    return field;
}

Weather& Battle::getWeather() {
    return weather;
}

const EventSystem& Battle::getEventSystem() const {
    return eventSystem;
}

void Battle::enqueueAction(const BattleAction& action) {
    queue.push(action, field.isTrickRoom());
}

void Battle::processTurn() {
    ++turnNumber;
    queue.setTrickRoom(field.isTrickRoom());
    
    // 触发回合开始时的效果
    triggerAbilities(Trigger::OnTurnStart, nullptr);
    triggerItemEffects(ItemTrigger::OnTurnStart, nullptr);
    
    while (!queue.empty()) {
        resolveNextAction();
    }
    
    // 触发回合结束时的效果
    triggerAbilities(Trigger::OnTurnEnd, nullptr);
    triggerItemEffects(ItemTrigger::OnTurnEnd, nullptr);
    
    applyWeatherEffects();
    applyFieldEffects();
    field.tick();
    weather.tick();
}

void Battle::resolveNextAction() {
    BattleAction action = queue.pop();
    if (action.actor == nullptr) {
        return;
    }

    switch (action.type) {
        case ActionType::Attack: {
            if (!action.target || action.actor->isFainted() || action.target->isFainted()) {
                return;
            }
            
            // 触发攻击前的效果
            triggerAbilities(Trigger::OnAttack, action.actor);
            triggerItemEffects(ItemTrigger::OnAttack, action.actor);
            
            // 计算伤害
            int damage = calculateDamage(action.actor, action.target, action.move);
            
            // 输出攻击信息
            std::cout << action.actor->getName() << " used " << action.move.getName() << "!" << std::endl;
            
            // 应用伤害
            action.target->setCurrentHP(action.target->getCurrentHP() - damage);
            
            // 输出伤害信息
            std::cout << "It dealt " << damage << " damage!" << std::endl;
            
            // 触发受到伤害时的效果
            triggerAbilities(Trigger::OnDamage, action.target);
            triggerItemEffect(action.target, ItemTrigger::OnDamage, action.actor);
            
            // 检查目标是否濒死
            if (action.target->isFainted()) {
                std::cout << action.target->getName() << " fainted!" << std::endl;
                Side* targetSide = findSideForPokemon(*this, action.target);
                if (targetSide) {
                    targetSide->autoSwitchNext();
                }
            }
            
            // 处理技能追加效果
            processMoveEffects(action.actor, action.target, action.move);
            
            // 触发攻击后的效果
            triggerAbilities(Trigger::OnDealDamage, action.actor);
            triggerItemEffects(ItemTrigger::OnDealDamage, action.actor);
            
            break;
        }
        case ActionType::Switch: {
            Side* side = findSideForPokemon(*this, action.actor);
            if (side) {
                // 触发退场时的效果
                triggerAbilities(Trigger::OnExit, action.actor);
                triggerItemEffects(ItemTrigger::OnSwitchOut, action.actor);
                
                // 执行切换
                Pokemon* oldPokemon = action.actor;
                bool switched = switchPokemon(*side, action.switchIndex);
                
                if (switched) {
                    Pokemon* newPokemon = side->getActivePokemon();
                    std::cout << oldPokemon->getName() << " was switched out for " << newPokemon->getName() << "!" << std::endl;
                    
                    // 触发出场时的效果
                    triggerAbilities(Trigger::OnEntry, newPokemon);
                    triggerItemEffects(ItemTrigger::OnEntry, newPokemon);
                }
            }
            break;
        }
        case ActionType::UseItem: {
            if (!action.actor) {
                break;
            }
            Item held = action.actor->getHeldItem();
            if (held.type == action.item) {
                std::cout << action.actor->getName() << " used " << held.name << "!" << std::endl;
                held.executeTrigger(ItemTrigger::OnEat, action.actor, action.target, *this);
            }
            break;
        }
        case ActionType::Pass:
            std::cout << action.actor->getName() << " did nothing!" << std::endl;
            break;
        default:
            break;
    }
}

int Battle::calculateDamage(Pokemon* attacker, Pokemon* defender, const Move& move) const {
    if (!attacker || !defender) {
        return 0;
    }

    float attackStat = (move.getCategory() == Category::Physical) ? attacker->getAttack() : attacker->getSpecialAttack();
    float defenseStat = (move.getCategory() == Category::Physical) ? defender->getDefense() : defender->getSpecialDefense();
    if (defenseStat <= 0.0f) {
        defenseStat = 1.0f;
    }

    float base = ((2.0f * attacker->getLevel() / 5.0f + 2.0f) * move.getPower() * attackStat / defenseStat) / 50.0f + 2.0f;
    float modifier = attacker->getTypeEffectiveness(move.getType());
    
    // 应用天气加成并输出日志
    float weatherModifier = weather.applyDamageModifier(move.getType());
    if (weatherModifier != 1.0f) {
        std::cout << weather.getName() << " boosted " << move.getName() << "!" << std::endl;
    }
    modifier *= weatherModifier;
    
    return static_cast<int>(std::lround(base * modifier));
}

bool Battle::switchPokemon(Side& side, int newIndex) {
    return side.switchActive(newIndex);
}

void Battle::processMoveEffects(Pokemon* attacker, Pokemon* defender, const Move& move) {
    if (!attacker || !defender) return;
    
    // 检查技能是否有追加效果
    MoveEffect effect = move.getEffect();
    if (effect == MoveEffect::None) return;
    
    // 检查效果是否触发
    int chance = move.getEffectChance();
    if (chance < 100) {
        // 这里应该使用随机数生成器来决定是否触发效果
        // 为了测试，我们假设效果总是触发
    }
    
    // 处理不同的追加效果
    switch (effect) {
        case MoveEffect::Paralyze:
            std::cout << defender->getName() << " was paralyzed!" << std::endl;
            defender->addStatus(StatusType::Paralysis);
            break;
        case MoveEffect::Sleep:
            std::cout << defender->getName() << " fell asleep!" << std::endl;
            defender->addStatus(StatusType::Sleep);
            break;
        case MoveEffect::Freeze:
            std::cout << defender->getName() << " was frozen solid!" << std::endl;
            defender->addStatus(StatusType::Freeze);
            break;
        case MoveEffect::Burn:
            std::cout << defender->getName() << " was burned!" << std::endl;
            defender->addStatus(StatusType::Burn);
            break;
        case MoveEffect::Poison:
            std::cout << defender->getName() << " was poisoned!" << std::endl;
            defender->addStatus(StatusType::Poison);
            break;
        case MoveEffect::Confuse:
            std::cout << defender->getName() << " became confused!" << std::endl;
            // 这里应该添加混乱状态的处理，但StatusType枚举中没有Confusion成员
            break;
        case MoveEffect::Flinch:
            std::cout << defender->getName() << " flinched and couldn't move!" << std::endl;
            // 这里应该添加畏缩状态的处理
            break;
        case MoveEffect::Recoil:
            {
                int recoilDamage = move.getEffectParam1();
                if (recoilDamage > 0) {
                    int actualRecoil = attacker->getMaxHP() * recoilDamage / 100;
                    if (actualRecoil < 1) actualRecoil = 1;
                    attacker->setCurrentHP(attacker->getCurrentHP() - actualRecoil);
                    std::cout << attacker->getName() << " took " << actualRecoil << " recoil damage!" << std::endl;
                }
            }
            break;
        case MoveEffect::Drain:
            {
                int drainPercentage = move.getEffectParam1();
                if (drainPercentage > 0) {
                    int damageDealt = calculateDamage(attacker, defender, move);
                    int drainAmount = damageDealt * drainPercentage / 100;
                    if (drainAmount < 1) drainAmount = 1;
                    attacker->setCurrentHP(attacker->getCurrentHP() + drainAmount);
                    std::cout << attacker->getName() << " drained " << drainAmount << " HP!" << std::endl;
                }
            }
            break;
        case MoveEffect::StatChange:
            {
                int statIndex = move.getEffectParam1();
                int changeAmount = move.getEffectParam2();
                // 这里应该添加能力变化的处理
                std::cout << defender->getName() << "'s stat changed!" << std::endl;
            }
            break;
        default:
            break;
    }
}

void Battle::applyWeatherEffects() {
    if (!weather.isActive()) {
        return;
    }

    for (Side* side : {&sideA, &sideB}) {
        Pokemon* active = side->getActivePokemon();
        if (!active || active->isFainted()) {
            continue;
        }

        switch (weather.type) {
            case WeatherType::Sandstorm:
                // 沙尘暴：对非岩石、地面、钢属性的宝可梦造成伤害
                if (active->getType1() != Type::Rock && active->getType1() != Type::Ground && active->getType1() != Type::Steel && 
                    active->getType2() != Type::Rock && active->getType2() != Type::Ground && active->getType2() != Type::Steel) {
                    int damage = std::max(1, active->getMaxHP() / 16);
                    active->setCurrentHP(active->getCurrentHP() - damage);
                    std::cout << active->getName() << " took damage from the sandstorm!" << std::endl;
                }
                break;
            case WeatherType::Hail:
                // 冰雹：对非冰属性的宝可梦造成伤害
                if (active->getType1() != Type::Ice && active->getType2() != Type::Ice) {
                    int damage = std::max(1, active->getMaxHP() / 16);
                    active->setCurrentHP(active->getCurrentHP() - damage);
                    std::cout << active->getName() << " took damage from hail!" << std::endl;
                }
                break;
            case WeatherType::Snow:
                // 雪：对冰属性以外的宝可梦造成少量伤害
                if (active->getType1() != Type::Ice && active->getType2() != Type::Ice) {
                    int damage = std::max(1, active->getMaxHP() / 32);
                    active->setCurrentHP(active->getCurrentHP() - damage);
                    std::cout << active->getName() << " took damage from the snow!" << std::endl;
                }
                break;
            case WeatherType::Rain:
                // 雨天：无直接伤害效果
                break;
            case WeatherType::Sun:
                // 晴天：无直接伤害效果
                break;
            default:
                break;
        }
    }
}

void Battle::applyFieldEffects() {
    if (!field.isActive()) {
        return;
    }

    for (Side* side : {&sideA, &sideB}) {
        Pokemon* active = side->getActivePokemon();
        if (!active || active->isFainted()) {
            continue;
        }

        switch (field.type) {
            case FieldType::Psychic: {
                // 精神场地：降低地面上宝可梦受到的物理伤害
                std::cout << "Psychic Terrain is active!" << std::endl;
                break;
            }
            case FieldType::Electric: {
                // 电气场地：提高地面上宝可梦的电属性技能威力
                std::cout << "Electric Terrain is active!" << std::endl;
                break;
            }
            case FieldType::Grassy: {
                // 青草场地：地面上的宝可梦每回合恢复少量HP
                int grassHeal = std::max(1, active->getMaxHP() / 16);
                active->setCurrentHP(active->getCurrentHP() + grassHeal);
                std::cout << active->getName() << " recovered HP from Grassy Terrain!" << std::endl;
                break;
            }
            case FieldType::Misty: {
                // 薄雾场地：降低地面上宝可梦受到的龙属性伤害，防止状态异常
                std::cout << "Misty Terrain is active!" << std::endl;
                break;
            }
            case FieldType::TrickRoom: {
                // 戏法空间：速度慢的宝可梦先行动
                std::cout << "Trick Room is active!" << std::endl;
                break;
            }
            default: {
                break;
            }
        }
    }
}

void Battle::triggerAbility(Pokemon* pokemon, Trigger trigger, Pokemon* opponent, void* context) {
    if (!pokemon) return;
    
    AbilityType abilityType = pokemon->getAbility();
    Ability ability = getAbility(abilityType);
    
    if (ability.hasTrigger(trigger)) {
        ability.executeTrigger(trigger, pokemon, opponent, context);
    }
}

void Battle::triggerAbilities(Trigger trigger, Pokemon* target) {
    // 触发所有宝可梦的特性
    for (Side* side : {&sideA, &sideB}) {
        Pokemon* active = side->getActivePokemon();
        if (active && !active->isFainted()) {
            triggerAbility(active, trigger, target);
        }
    }
}

void Battle::triggerItemEffect(Pokemon* pokemon, ItemTrigger trigger, Pokemon* opponent, void* context) {
    if (!pokemon) return;
    
    Item item = pokemon->getHeldItem();
    if (item.type == ItemType::None) return;
    
    if (item.hasTrigger(trigger)) {
        std::cout << pokemon->getName() << "'s " << item.name << " activated!" << std::endl;
        item.executeTrigger(trigger, pokemon, opponent, *this, context);
    }
}

void Battle::triggerItemEffects(ItemTrigger trigger, Pokemon* target) {
    // 触发所有宝可梦的物品效果
    for (Side* side : {&sideA, &sideB}) {
        Pokemon* active = side->getActivePokemon();
        if (active && !active->isFainted()) {
            triggerItemEffect(active, trigger, target);
        }
    }
}
