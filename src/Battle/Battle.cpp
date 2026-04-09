#include "Battle/Battle.h"
#include "Battle/Items.h"
#include "Battle/PRNG.h"
#include "Battle/BattleToJson.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <array>
#include <limits>

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
    BattleAction adjusted = action;
    if (adjusted.type == ActionType::Attack && adjusted.actor && adjusted.actor->getItemType() == ItemType::QuickClaw) {
        if (PRNG::nextFloat(0.0f, 1.0f) < 0.2f) {
            adjusted.priority = std::numeric_limits<int>::max() / 4;
        }
    }
    queue.push(adjusted, field.isTrickRoom());
}

void Battle::processTurn() {
    ++turnNumber;
    queue.setTrickRoom(field.isTrickRoom());
    
    // 输出回合开始的Json
    json turnStartJson = BattleToJson::battleToJson(*this);
    turnStartJson["event"] = "turn_start";
    
    // 写入到cache文件
    BattleToJson::writeToCache(turnStartJson, "battle_turn_start_" + std::to_string(turnNumber) + ".json");
    
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
    
    // 输出回合结束的Json
    json turnEndJson = BattleToJson::battleToJson(*this);
    turnEndJson["event"] = "turn_end";
    
    // 写入到cache文件
    BattleToJson::writeToCache(turnEndJson, "battle_turn_end_" + std::to_string(turnNumber) + ".json");
    
    // 重置所有宝可梦的保护状态（但不重置保护计数）
    Pokemon* activeA = sideA.getActivePokemon();
    if (activeA) {
        activeA->setIsProtected(false);
    }
    
    Pokemon* activeB = sideB.getActivePokemon();
    if (activeB) {
        activeB->setIsProtected(false);
    }
}

void Battle::resolveNextAction() {
    BattleAction action = queue.pop();
    if (action.actor == nullptr) {
        return;
    }
    
    // 输出行动开始的Json
    json actionStartJson = BattleToJson::actionToJson(action);
    actionStartJson["event"] = "action_start";
    
    // 写入到cache文件
    BattleToJson::writeToCache(actionStartJson, "battle_action_start_" + std::to_string(turnNumber) + "_" + actionStartJson["type"].get<std::string>() + ".json");

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
            ItemDamageContext damageContext;
            damageContext.move = &action.move;
            damageContext.damage = damage;
            damageContext.hpBeforeDamage = action.target->getCurrentHP();
            damageContext.wasSuperEffective = action.target->getTypeEffectiveness(action.move.getType()) > 1.0f;
            damageContext.isDamagingMove = action.move.getCategory() != Category::Status;
            damageContext.isContact = action.move.getCategory() == Category::Physical;
            
            // 输出攻击信息
            // 应用伤害
            action.target->setCurrentHP(action.target->getCurrentHP() - damage);
            
            // 触发受到伤害时的效果
            triggerAbilities(Trigger::OnDamage, action.target);
            triggerItemEffect(action.target, ItemTrigger::OnDamage, action.actor, &damageContext);
            
            // 检查目标是否濒死
            if (action.target->isFainted()) {
                Side* targetSide = findSideForPokemon(*this, action.target);
                if (targetSide) {
                    // 宝可梦死亡时重置保护计数
                    targetSide->resetProtectCount();
                    targetSide->autoSwitchNext();
                }
            }
            
            // 处理技能追加效果
            processMoveEffects(action.actor, action.target, action.move);
            
            // 触发攻击后的效果
            triggerAbilities(Trigger::OnDealDamage, action.actor);
            triggerItemEffect(action.actor, ItemTrigger::OnDealDamage, action.target, &damageContext);
            
            // 输出攻击结果的Json
            json attackResultJson = BattleToJson::battleToJson(*this);
            attackResultJson["event"] = "attack_result";
            attackResultJson["actor"] = action.actor->getName();
            attackResultJson["target"] = action.target->getName();
            attackResultJson["move"] = action.move.getName();
            
            // 写入到cache文件
            BattleToJson::writeToCache(attackResultJson, "battle_attack_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + "_" + action.move.getName() + ".json");
            
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
                    
                    // 触发出场时的效果
                    // 获取对手的活跃宝可梦
                    Pokemon* opponentPokemon = (side == &sideA) ? sideB.getActivePokemon() : sideA.getActivePokemon();
                    triggerAbility(newPokemon, Trigger::OnEntry, opponentPokemon);
                    triggerItemEffect(newPokemon, ItemTrigger::OnEntry, opponentPokemon);
                    
                    // 输出切换结果的Json
                    json switchResultJson = BattleToJson::battleToJson(*this);
                    switchResultJson["event"] = "switch_result";
                    switchResultJson["old_pokemon"] = oldPokemon->getName();
                    switchResultJson["new_pokemon"] = newPokemon->getName();
                    
                    // 写入到cache文件
                    BattleToJson::writeToCache(switchResultJson, "battle_switch_result_" + std::to_string(turnNumber) + "_" + oldPokemon->getName() + "_" + newPokemon->getName() + ".json");
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
                held.executeTrigger(ItemTrigger::OnEat, action.actor, action.target, *this);
                
                // 输出使用物品结果的Json
                json itemResultJson = BattleToJson::battleToJson(*this);
                itemResultJson["event"] = "item_result";
                itemResultJson["actor"] = action.actor->getName();
                itemResultJson["item"] = BattleToJson::itemTypeToString(action.item);
                
                // 写入到cache文件
                BattleToJson::writeToCache(itemResultJson, "battle_item_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + ".json");
            }
            break;
        }
        case ActionType::Pass: {
            
            // 输出跳过结果的Json
            json passResultJson = BattleToJson::battleToJson(*this);
            passResultJson["event"] = "pass_result";
            passResultJson["actor"] = action.actor->getName();
            
            // 写入到cache文件
            BattleToJson::writeToCache(passResultJson, "battle_pass_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + ".json");
            break;
        }
        default:
            break;
    }
}

int Battle::calculateDamage(Pokemon* attacker, Pokemon* defender, const Move& move) const {
    if (!attacker || !defender) {
        return 0;
    }

    // 检查目标是否处于保护状态
    if (defender->getIsProtected()) {
        return 0;
    }

    const AbilityType atkAbility = attacker->getAbility();
    const AbilityType defAbility = defender->getAbility();

    // 当前引擎可稳定实现的属性免疫/吸收能力。
    if (defAbility == AbilityType::Levitate && move.getType() == Type::Ground) {
        return 0;
    }
    if (defAbility == AbilityType::WaterAbsorb && move.getType() == Type::Water) {
        defender->setCurrentHP(defender->getCurrentHP() + std::max(1, defender->getMaxHP() / 4));
        return 0;
    }
    if (defAbility == AbilityType::VoltAbsorb && move.getType() == Type::Electric) {
        defender->setCurrentHP(defender->getCurrentHP() + std::max(1, defender->getMaxHP() / 4));
        return 0;
    }
    if (defAbility == AbilityType::FlashFire && move.getType() == Type::Fire) {
        return 0;
    }

    auto stageMultiplier = [](int stage) -> float {
        if (stage >= 0) {
            return static_cast<float>(2 + stage) / 2.0f;
        }
        return 2.0f / static_cast<float>(2 - stage);
    };

    float attackStat = (move.getCategory() == Category::Physical) ? attacker->getAttack() : attacker->getSpecialAttack();
    float defenseStat = (move.getCategory() == Category::Physical) ? defender->getDefense() : defender->getSpecialDefense();

    if (move.getCategory() == Category::Physical) {
        attackStat *= stageMultiplier(attacker->getStatStage(StatIndex::Attack));
        defenseStat *= stageMultiplier(defender->getStatStage(StatIndex::Defense));
    } else if (move.getCategory() == Category::Special) {
        attackStat *= stageMultiplier(attacker->getStatStage(StatIndex::SpecialAttack));
        defenseStat *= stageMultiplier(defender->getStatStage(StatIndex::SpecialDefense));
    }

    if (defenseStat <= 0.0f) {
        defenseStat = 1.0f;
    }

    float base = ((2.0f * attacker->getLevel() / 5.0f + 2.0f) * move.getPower() * attackStat / defenseStat) / 50.0f + 2.0f;
    float modifier = defender->getTypeEffectiveness(move.getType());
    
    // 应用天气加成并输出日志
    float weatherModifier = weather.applyDamageModifier(move.getType());
    modifier *= weatherModifier;
    
    float damage = base * modifier;

    if (atkAbility == AbilityType::Blaze && move.getType() == Type::Fire && attacker->getCurrentHP() * 3 <= attacker->getMaxHP()) {
        damage *= 1.5f;
    }
    if (atkAbility == AbilityType::Torrent && move.getType() == Type::Water && attacker->getCurrentHP() * 3 <= attacker->getMaxHP()) {
        damage *= 1.5f;
    }
    if (atkAbility == AbilityType::Overgrow && move.getType() == Type::Grass && attacker->getCurrentHP() * 3 <= attacker->getMaxHP()) {
        damage *= 1.5f;
    }
    if (defAbility == AbilityType::Multiscale && defender->getCurrentHP() == defender->getMaxHP()) {
        damage *= 0.5f;
    }
    if (atkAbility == AbilityType::FlashFire && move.getType() == Type::Fire) {
        damage *= 1.5f;
    }

    // 应用攻击方/防守方道具的伤害修正
    const Item attackerItem = attacker->getHeldItem();
    damage = attackerItem.applyDamageModifier(damage, attacker, defender, move, true);
    const Item defenderItem = defender->getHeldItem();
    damage = defenderItem.applyDamageModifier(damage, defender, attacker, move, false);

    // 添加随机数影响，伤害在85%到100%之间波动
    float randomFactor = PRNG::nextFloat(0.85f, 1.0f);
    int finalDamage = static_cast<int>(std::lround(damage * randomFactor));
    if (finalDamage < 0) {
        finalDamage = 0;
    }
    return finalDamage;
}

bool Battle::switchPokemon(Side& side, int newIndex) {
    bool switched = side.switchActive(newIndex);
    if (switched) {
        // 切换宝可梦时重置保护计数
        side.resetProtectCount();
    }
    return switched;
}

void Battle::processMoveEffects(Pokemon* attacker, Pokemon* defender, const Move& move) {
    if (!attacker || !defender) return;
    
    // 检查技能是否有追加效果
    MoveEffect effect = move.getEffect();
    if (effect == MoveEffect::None) return;
    
    // 检查效果是否触发
    int chance = move.getEffectChance();
    if (chance < 100) {
        if (PRNG::nextFloat(0.0f, 100.0f) > static_cast<float>(chance)) {
            return;
        }
    }

    if (move.getName() == "Dragon Dance") {
        attacker->changeStatStage(StatIndex::Attack, 1);
        attacker->changeStatStage(StatIndex::Speed, 1);
        triggerItemEffect(attacker, ItemTrigger::OnStatChange, defender);
        return;
    }
    
    // 处理不同的追加效果
    switch (effect) {
        case MoveEffect::Paralyze:
            defender->addStatus(StatusType::Paralysis);
            break;
        case MoveEffect::Sleep:
            defender->addStatus(StatusType::Sleep);
            break;
        case MoveEffect::Freeze:
            defender->addStatus(StatusType::Freeze);
            break;
        case MoveEffect::Burn:
            defender->addStatus(StatusType::Burn);
            break;
        case MoveEffect::Poison:
            if (move.getEffectParam1() == 2) {
                defender->addStatus(StatusType::ToxicPoison);
            } else {
                defender->addStatus(StatusType::Poison);
            }
            break;
        case MoveEffect::Confuse:
            defender->addStatus(StatusType::Confusion);
            break;
        case MoveEffect::Flinch:
            // 这里应该添加畏缩状态的处理
            break;
        case MoveEffect::Recoil:
            {
                int recoilDamage = move.getEffectParam1();
                if (recoilDamage > 0) {
                    int actualRecoil = attacker->getMaxHP() * recoilDamage / 100;
                    if (actualRecoil < 1) actualRecoil = 1;
                    attacker->setCurrentHP(attacker->getCurrentHP() - actualRecoil);
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
                }
            }
            break;
        case MoveEffect::StatChange:
            {
                const int encodedIndex = move.getEffectParam1();
                const int changeAmount = move.getEffectParam2();
                const bool affectSelf = encodedIndex < 0;
                const int absIndex = std::abs(encodedIndex);
                if (changeAmount == 0 || absIndex <= 0 || absIndex >= static_cast<int>(StatIndex::Count)) {
                    break;
                }

                Pokemon* target = affectSelf ? attacker : defender;
                Pokemon* itemOpponent = affectSelf ? defender : attacker;
                const int beforeStage = target->getStatStage(static_cast<StatIndex>(absIndex));
                target->changeStatStage(static_cast<StatIndex>(absIndex), changeAmount);
                const int afterStage = target->getStatStage(static_cast<StatIndex>(absIndex));
                if (afterStage != beforeStage) {
                    triggerItemEffect(target, ItemTrigger::OnStatChange, itemOpponent);
                }
            }
            break;
        case MoveEffect::Safeguard:
            {
                // 获取攻击者所在的Side
                Side* side = findSideForPokemon(*this, attacker);
                if (!side) break;
                
                // 计算保护的成功率
                int protectCount = side->getProtectCount();
                float successRate = 1.0f;
                if (protectCount > 0) {
                    // 连续使用保护，成功率降低（每多一次减少2/3的成功率）
                    for (int i = 0; i < protectCount; i++) {
                        successRate *= (1.0f / 3.0f);
                    }
                }
                
                // 使用随机数生成器来决定是否成功
                bool success = PRNG::nextFloat(0.0f, 1.0f) <= successRate;
                
                if (success) {
                    attacker->setIsProtected(true);
                    side->setProtectCount(protectCount + 1);
                    std::cerr << attacker->getName() << " used Protect!" << std::endl;
                } else {
                    // 守护失败，将计数器调回0
                    side->resetProtectCount();
                    std::cerr << attacker->getName() << "'s Protect failed!" << std::endl;
                }
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
                }
                break;
            case WeatherType::Hail:
                // 冰雹：对非冰属性的宝可梦造成伤害
                if (active->getType1() != Type::Ice && active->getType2() != Type::Ice) {
                    int damage = std::max(1, active->getMaxHP() / 16);
                    active->setCurrentHP(active->getCurrentHP() - damage);
                }
                break;
            case WeatherType::Snow:
                // 雪：对冰属性以外的宝可梦造成少量伤害
                if (active->getType1() != Type::Ice && active->getType2() != Type::Ice) {
                    int damage = std::max(1, active->getMaxHP() / 32);
                    active->setCurrentHP(active->getCurrentHP() - damage);
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
                break;
            }
            case FieldType::Electric: {
                // 电气场地：提高地面上宝可梦的电属性技能威力
                break;
            }
            case FieldType::Grassy: {
                // 青草场地：地面上的宝可梦每回合恢复少量HP
                int grassHeal = std::max(1, active->getMaxHP() / 16);
                active->setCurrentHP(active->getCurrentHP() + grassHeal);
                break;
            }
            case FieldType::Misty: {
                // 薄雾场地：降低地面上宝可梦受到的龙属性伤害，防止状态异常
                break;
            }
            case FieldType::TrickRoom: {
                // 戏法空间：速度慢的宝可梦先行动
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

    auto snapshotStages = [](Pokemon* target) {
        std::array<int, 5> stages = {0, 0, 0, 0, 0};
        if (!target) {
            return stages;
        }
        stages[0] = target->getStatStage(StatIndex::Attack);
        stages[1] = target->getStatStage(StatIndex::Defense);
        stages[2] = target->getStatStage(StatIndex::SpecialAttack);
        stages[3] = target->getStatStage(StatIndex::SpecialDefense);
        stages[4] = target->getStatStage(StatIndex::Speed);
        return stages;
    };

    auto statChanged = [](Pokemon* target, const std::array<int, 5>& before) {
        if (!target) {
            return false;
        }
        return target->getStatStage(StatIndex::Attack) != before[0]
            || target->getStatStage(StatIndex::Defense) != before[1]
            || target->getStatStage(StatIndex::SpecialAttack) != before[2]
            || target->getStatStage(StatIndex::SpecialDefense) != before[3]
            || target->getStatStage(StatIndex::Speed) != before[4];
    };

    const std::array<int, 5> selfBefore = snapshotStages(pokemon);
    const std::array<int, 5> opponentBefore = snapshotStages(opponent);
    
    AbilityType abilityType = pokemon->getAbility();
    Ability ability = getAbility(abilityType);
    
    if (ability.hasTrigger(trigger)) {
        ability.executeTrigger(trigger, pokemon, opponent, context);
    }

    if (statChanged(pokemon, selfBefore)) {
        triggerItemEffect(pokemon, ItemTrigger::OnStatChange, opponent);
    }
    if (statChanged(opponent, opponentBefore)) {
        triggerItemEffect(opponent, ItemTrigger::OnStatChange, pokemon);
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
