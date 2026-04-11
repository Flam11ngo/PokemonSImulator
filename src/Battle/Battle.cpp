#include "Battle/Battle.h"
#include "Battle/Items.h"
#include "Battle/MoveEffectHandler.h"
#include "Battle/PRNG.h"
#include "Battle/BattleToJson.h"
#include <cmath>
#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <optional>
#include <unordered_map>

namespace {
std::string normalizeMoveName(const std::string& moveName) {
    std::string normalized;
    normalized.reserve(moveName.size());
    for (char ch : moveName) {
        if (ch == ' ' || ch == '-' || ch == '\'' || ch == '_') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

bool moveNameEquals(const Move& move, const char* expectedLowerNoPunct) {
    return normalizeMoveName(move.getName()) == expectedLowerNoPunct;
}

bool isTwoTurnSemiInvulnerableMove(const Move& move) {
    return moveNameEquals(move, "dig") || moveNameEquals(move, "fly") || moveNameEquals(move, "bounce")
        || moveNameEquals(move, "dive") || moveNameEquals(move, "phantomforce")
        || moveNameEquals(move, "shadowforce");
}

Battle::SemiInvulnerableState stateForTwoTurnMove(const Move& move) {
    if (moveNameEquals(move, "dig")) {
        return Battle::SemiInvulnerableState::Underground;
    }
    if (moveNameEquals(move, "fly") || moveNameEquals(move, "bounce")) {
        return Battle::SemiInvulnerableState::Airborne;
    }
    if (moveNameEquals(move, "dive")) {
        return Battle::SemiInvulnerableState::Underwater;
    }
    if (moveNameEquals(move, "phantomforce") || moveNameEquals(move, "shadowforce")) {
        return Battle::SemiInvulnerableState::Phased;
    }
    return Battle::SemiInvulnerableState::None;
}

struct MultiHitConfig {
    int minHits;
    int maxHits;
    bool stopOnMiss;
};

std::optional<MultiHitConfig> getMultiHitConfig(const Move& move) {
    if (moveNameEquals(move, "populationbomb")) {
        return MultiHitConfig{10, 10, true};
    }

    if (moveNameEquals(move, "doublehit") || moveNameEquals(move, "twineedle") || moveNameEquals(move, "bonemerang")
        || moveNameEquals(move, "dualchop") || moveNameEquals(move, "dualwingbeat")
        || moveNameEquals(move, "doubleironbash") || moveNameEquals(move, "geargrind")) {
        return MultiHitConfig{2, 2, false};
    }

    if (moveNameEquals(move, "tripledive") || moveNameEquals(move, "triplekick") || moveNameEquals(move, "tripleaxel")
        || moveNameEquals(move, "surgingstrikes")) {
        return MultiHitConfig{3, 3, false};
    }

    if (moveNameEquals(move, "doubleslap") || moveNameEquals(move, "cometpunch") || moveNameEquals(move, "furyattack")
        || moveNameEquals(move, "pinmissile") || moveNameEquals(move, "iciclespear") || moveNameEquals(move, "rockblast")
        || moveNameEquals(move, "armthrust") || moveNameEquals(move, "tailslap") || moveNameEquals(move, "bulletseed")
        || moveNameEquals(move, "bonerush") || moveNameEquals(move, "watershuriken") || moveNameEquals(move, "barrage")
        || moveNameEquals(move, "furyswipes") || moveNameEquals(move, "spikecannon")
        || moveNameEquals(move, "scaleshot")) {
        return MultiHitConfig{2, 5, false};
    }

    return std::nullopt;
}

int rollHitCount(const MultiHitConfig& config) {
    if (config.minHits >= config.maxHits) {
        return config.minHits;
    }

    if (config.minHits == 2 && config.maxHits == 5) {
        const int roll = static_cast<int>(PRNG::nextFloat(0.0f, 100.0f));
        if (roll < 35) return 2;
        if (roll < 70) return 3;
        if (roll < 85) return 4;
        return 5;
    }

    const int span = config.maxHits - config.minHits + 1;
    const int roll = static_cast<int>(PRNG::nextFloat(0.0f, static_cast<float>(span)));
    return config.minHits + std::min(span - 1, std::max(0, roll));
}

int heavySlamPowerByWeightRatio(int attackerWeight, int defenderWeight) {
    const int safeDefenderWeight = std::max(1, defenderWeight);
    const float ratio = static_cast<float>(attackerWeight) / static_cast<float>(safeDefenderWeight);
    if (ratio >= 5.0f) return 120;
    if (ratio >= 4.0f) return 100;
    if (ratio >= 3.0f) return 80;
    if (ratio >= 2.0f) return 60;
    return 40;
}

float stageMultiplier(int stage) {
    if (stage >= 0) {
        return static_cast<float>(2 + stage) / 2.0f;
    }
    return 2.0f / static_cast<float>(2 - stage);
}

bool isStatusInflictingEffect(MoveEffect effect) {
    switch (effect) {
        case MoveEffect::Paralyze:
        case MoveEffect::Sleep:
        case MoveEffect::Freeze:
        case MoveEffect::Burn:
        case MoveEffect::Poison:
        case MoveEffect::Confuse:
            return true;
        default:
            return false;
    }
}

bool hasMajorStatus(const Pokemon* pokemon) {
    if (!pokemon) {
        return false;
    }
    return pokemon->hasStatus(StatusType::Burn)
        || pokemon->hasStatus(StatusType::Freeze)
        || pokemon->hasStatus(StatusType::Paralysis)
        || pokemon->hasStatus(StatusType::Poison)
        || pokemon->hasStatus(StatusType::Sleep)
        || pokemon->hasStatus(StatusType::ToxicPoison);
}

bool isBerryItem(ItemType itemType) {
    const int value = static_cast<int>(itemType);
    return value >= static_cast<int>(ItemType::OranBerry)
        && value <= static_cast<int>(ItemType::RowapBerry);
}

bool isSheerForceBoostedMove(const Move& move) {
    if (move.getCategory() == Category::Status || move.getEffectChance() <= 0) {
        return false;
    }

    switch (move.getEffect()) {
        case MoveEffect::Burn:
        case MoveEffect::Poison:
        case MoveEffect::Paralyze:
        case MoveEffect::Freeze:
        case MoveEffect::Flinch:
        case MoveEffect::Confuse:
            return true;
        case MoveEffect::StatChange:
            return move.getEffectParam1() > 0;
        default:
            return false;
    }
}

bool isCrashRecoilMove(const Move& move) {
    return moveNameEquals(move, "jumpkick") || moveNameEquals(move, "highjumpkick");
}

bool isSelfStatDropMove(const Move& move) {
    return moveNameEquals(move, "closecombat") || moveNameEquals(move, "superpower")
        || moveNameEquals(move, "overheat") || moveNameEquals(move, "leafstorm")
        || moveNameEquals(move, "dracometeor") || moveNameEquals(move, "fleurcannon")
        || moveNameEquals(move, "steelbeam") || moveNameEquals(move, "vcreate")
        || moveNameEquals(move, "clangoroussoulblaze");
}

void applyCrashRecoil(Pokemon* attacker, const Move& move) {
    if (!attacker || !isCrashRecoilMove(move)) {
        return;
    }

    const int recoilDamage = std::max(1, attacker->getMaxHP() / 2);
    attacker->setCurrentHP(attacker->getCurrentHP() - recoilDamage);
}

void applySelfStatDropMove(Pokemon* attacker, const Move& move) {
    if (!attacker || !isSelfStatDropMove(move)) {
        return;
    }

    if (moveNameEquals(move, "closecombat") || moveNameEquals(move, "superpower")) {
        attacker->changeStatStage(StatIndex::Attack, -1);
        attacker->changeStatStage(StatIndex::Defense, -1);
        return;
    }

    if (moveNameEquals(move, "overheat") || moveNameEquals(move, "leafstorm") || moveNameEquals(move, "dracometeor")
        || moveNameEquals(move, "fleurcannon")) {
        attacker->changeStatStage(StatIndex::SpecialAttack, -2);
        return;
    }

    if (moveNameEquals(move, "steelbeam")) {
        attacker->changeStatStage(StatIndex::SpecialAttack, -1);
        return;
    }

    if (moveNameEquals(move, "vcreate")) {
        attacker->changeStatStage(StatIndex::Defense, -1);
        attacker->changeStatStage(StatIndex::SpecialDefense, -1);
        attacker->changeStatStage(StatIndex::Speed, -1);
        return;
    }

    if (moveNameEquals(move, "clangoroussoulblaze")) {
        attacker->changeStatStage(StatIndex::Attack, -1);
        attacker->changeStatStage(StatIndex::Defense, -1);
        attacker->changeStatStage(StatIndex::SpecialAttack, -1);
        attacker->changeStatStage(StatIndex::SpecialDefense, -1);
        attacker->changeStatStage(StatIndex::Speed, -1);
    }
}

bool doesMoveHit(const Pokemon* attacker, const Pokemon* defender, const Move& move) {
    if (!attacker || !defender) {
        return false;
    }

    const int baseAccuracy = move.getAccuracy();
    if (baseAccuracy <= 0) {
        return false;
    }

    if (baseAccuracy >= 100 && attacker->getAccuracyStage() == 0 && defender->getEvasionStage() == 0) {
        return true;
    }

    float adjustedAccuracy = static_cast<float>(baseAccuracy);
    adjustedAccuracy *= stageMultiplier(attacker->getAccuracyStage() - defender->getEvasionStage());
    if (adjustedAccuracy > 100.0f) {
        adjustedAccuracy = 100.0f;
    }
    if (adjustedAccuracy < 1.0f) {
        adjustedAccuracy = 1.0f;
    }

    return PRNG::nextFloat(0.0f, 100.0f) <= adjustedAccuracy;
}

void clearVolatileSwitchState(Pokemon* pokemon) {
    if (!pokemon) {
        return;
    }

    pokemon->clearSubstitute();
    pokemon->clearLeechSeedSource();
    if (pokemon->getAccuracyStage() != 0) {
        pokemon->changeAccuracyStage(-pokemon->getAccuracyStage());
    }
    if (pokemon->getEvasionStage() != 0) {
        pokemon->changeEvasionStage(-pokemon->getEvasionStage());
    }
}

Type weatherBallType(const Weather& weather) {
    if (!weather.isActive()) {
        return Type::Normal;
    }
    switch (weather.type) {
        case WeatherType::Rain: return Type::Water;
        case WeatherType::Sun: return Type::Fire;
        case WeatherType::Sandstorm: return Type::Rock;
        case WeatherType::Hail:
        case WeatherType::Snow:
            return Type::Ice;
        default:
            return Type::Normal;
    }
}

void resetPokemonStatStages(Pokemon* pokemon) {
    if (!pokemon) {
        return;
    }

    constexpr std::array<StatIndex, 5> kResettableStats = {
        StatIndex::Attack,
        StatIndex::Defense,
        StatIndex::SpecialAttack,
        StatIndex::SpecialDefense,
        StatIndex::Speed,
    };

    for (StatIndex stat : kResettableStats) {
        const int currentStage = pokemon->getStatStage(stat);
        if (currentStage != 0) {
            pokemon->changeStatStage(stat, -currentStage);
        }
    }
}

struct ScopedActionEventEmitter {
    EventSystem* eventSystem = nullptr;
    int turnNumber = 0;
    BattleAction* action = nullptr;
    bool active = false;

    ~ScopedActionEventEmitter() {
        if (!active || !eventSystem || !action) {
            return;
        }

        eventSystem->emitActionEvent(RuntimeActionEvent{
            RuntimeEventPhase::AfterActionResolve,
            turnNumber,
            *action,
        });
    }
};
}  // namespace

const Side* Battle::findSideForPokemon(const Battle& battle, Pokemon* pokemon) {
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

Side* Battle::findSideForPokemon(Battle& battle, Pokemon* pokemon) {
    return const_cast<Side*>(findSideForPokemon(static_cast<const Battle&>(battle), pokemon));
}

Battle::Battle(Side sideA, Side sideB)
    : sideA(std::move(sideA)), sideB(std::move(sideB)), field(), weather(), queue(), eventSystem(), turnNumber(0) {
    initializeCoreMoveRules();
    clearSideRuntimeState(this->sideA);
    clearSideRuntimeState(this->sideB);
    runtimeMoveState.roundUsedThisTurn = false;

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

void Battle::initializeCoreMoveRules() {
    auto registerProtectLikeRule = [this](const std::string& name) {
        moveRuleRegistry.registerRule(name, [](Battle& battle, Pokemon* attacker, Pokemon*, const Move&) {
            Side* actorSide = Battle::findSideForPokemon(battle, attacker);
            if (!actorSide) {
                return true;
            }

            const int protectCount = actorSide->getProtectCount();
            float successRate = 1.0f;
            if (protectCount > 0) {
                for (int i = 0; i < protectCount; ++i) {
                    successRate *= (1.0f / 3.0f);
                }
            }

            const bool success = PRNG::nextFloat(0.0f, 1.0f) <= successRate;
            if (success) {
                attacker->setIsProtected(true);
                actorSide->setProtectCount(protectCount + 1);
            } else {
                actorSide->resetProtectCount();
            }
            return true;
        });
    };

    auto registerRecoverLikeRule = [this](const std::string& name) {
        moveRuleRegistry.registerRule(name, [](Battle&, Pokemon* attacker, Pokemon*, const Move&) {
            const int healAmount = std::max(1, attacker->getMaxHP() / 2);
            attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
            return true;
        });
    };

    auto registerEvasionDropRule = [this](const std::string& name) {
        moveRuleRegistry.registerRule(name, [](Battle&, Pokemon* attacker, Pokemon* defender, const Move&) {
            if (defender->getSubstituteHP() > 0) {
                return true;
            }

            const AbilityType defenderAbility = defender->getAbility();
            if (defenderAbility == AbilityType::KeenEye || defenderAbility == AbilityType::ClearBody || defenderAbility == AbilityType::WhiteSmoke) {
                return true;
            }
            if (defenderAbility == AbilityType::MirrorArmor && attacker) {
                attacker->changeEvasionStage(1);
                return true;
            }

            defender->changeEvasionStage(1);
            return true;
        });
    };

    moveRuleRegistry.registerRule("trickroom", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        if (battle.getField().isTrickRoom()) {
            battle.getField().setField(FieldType::None, 0);
        } else {
            battle.getField().setField(FieldType::TrickRoom, 5);
        }
        return true;
    });

    registerProtectLikeRule("protect");
    registerProtectLikeRule("detect");

    moveRuleRegistry.registerRule("raindance", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getWeather().setWeather(WeatherType::Rain, 5);
        return true;
    });
    moveRuleRegistry.registerRule("sunnyday", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getWeather().setWeather(WeatherType::Sun, 5);
        return true;
    });
    moveRuleRegistry.registerRule("sandstorm", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getWeather().setWeather(WeatherType::Sandstorm, 5);
        return true;
    });
    moveRuleRegistry.registerRule("hail", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getWeather().setWeather(WeatherType::Hail, 5);
        return true;
    });
    moveRuleRegistry.registerRule("snowscape", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getWeather().setWeather(WeatherType::Hail, 5);
        return true;
    });

    moveRuleRegistry.registerRule("psychicterrain", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getField().setField(FieldType::Psychic, 5);
        return true;
    });
    moveRuleRegistry.registerRule("electricterrain", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getField().setField(FieldType::Electric, 5);
        return true;
    });
    moveRuleRegistry.registerRule("grassyterrain", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getField().setField(FieldType::Grassy, 5);
        return true;
    });
    moveRuleRegistry.registerRule("mistyterrain", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        battle.getField().setField(FieldType::Misty, 5);
        return true;
    });

    registerRecoverLikeRule("recover");
    registerRecoverLikeRule("softboiled");
    registerRecoverLikeRule("milkdrink");

    moveRuleRegistry.registerRule("rest", [](Battle&, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }

        if (attacker->getAbility() == AbilityType::Insomnia || attacker->getAbility() == AbilityType::VitalSpirit) {
            return true;
        }

        attacker->setCurrentHP(attacker->getMaxHP());
        attacker->addStatus(StatusType::Sleep);
        return true;
    });

    moveRuleRegistry.registerRule("haze", [](Battle& battle, Pokemon*, Pokemon*, const Move&) {
        resetPokemonStatStages(battle.getSideA().getActivePokemon());
        resetPokemonStatStages(battle.getSideB().getActivePokemon());
        return true;
    });

    moveRuleRegistry.registerRule("reflect", [](Battle& battle, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = Battle::findSideForPokemon(battle, attacker);
        if (actorSide) {
            actorSide->setReflectTurns(5);
        }
        return true;
    });
    moveRuleRegistry.registerRule("lightscreen", [](Battle& battle, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = Battle::findSideForPokemon(battle, attacker);
        if (actorSide) {
            actorSide->setLightScreenTurns(5);
        }
        return true;
    });

    moveRuleRegistry.registerRule("spikes", [](Battle& battle, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = Battle::findSideForPokemon(battle, attacker);
        if (!actorSide) {
            return true;
        }
        Side& opponentSide = battle.getOpponentSide(*actorSide);
        opponentSide.addSpikesLayer();
        return true;
    });
    moveRuleRegistry.registerRule("toxicspikes", [](Battle& battle, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = Battle::findSideForPokemon(battle, attacker);
        if (!actorSide) {
            return true;
        }
        Side& opponentSide = battle.getOpponentSide(*actorSide);
        opponentSide.addToxicSpikesLayer();
        return true;
    });
    moveRuleRegistry.registerRule("stealthrock", [](Battle& battle, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = Battle::findSideForPokemon(battle, attacker);
        if (!actorSide) {
            return true;
        }
        Side& opponentSide = battle.getOpponentSide(*actorSide);
        opponentSide.setStealthRock(true);
        return true;
    });

    moveRuleRegistry.registerRule("leechseed", [](Battle&, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (defender->getSubstituteHP() > 0) {
            return true;
        }
        if (defender->getType1() == Type::Grass || defender->getType2() == Type::Grass) {
            return true;
        }
        defender->setLeechSeedSource(attacker);
        return true;
    });
    moveRuleRegistry.registerRule("substitute", [](Battle&, Pokemon* attacker, Pokemon*, const Move&) {
        if (attacker->getSubstituteHP() > 0) {
            return true;
        }

        const int substituteCost = std::max(1, attacker->getMaxHP() / 4);
        if (attacker->getCurrentHP() <= substituteCost) {
            return true;
        }

        attacker->setCurrentHP(attacker->getCurrentHP() - substituteCost);
        attacker->setSubstituteHP(substituteCost);
        return true;
    });

    registerEvasionDropRule("sandattack");
    registerEvasionDropRule("smokescreen");
    registerEvasionDropRule("flash");

    moveRuleRegistry.registerRule("closecombat", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("superpower", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("overheat", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("leafstorm", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("dracometeor", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("steelbeam", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("vcreate", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("fleurcannon", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
    moveRuleRegistry.registerRule("clangoroussoulblaze", [](Battle&, Pokemon* attacker, Pokemon*, const Move& move) {
        applySelfStatDropMove(attacker, move);
        return true;
    });
}

void Battle::clearPokemonRuntimeState(Pokemon* pokemon) {
    if (!pokemon) {
        return;
    }

    runtimeMoveState.lastUsedMoveName.erase(pokemon);
    runtimeMoveState.encoreState.erase(pokemon);
    runtimeMoveState.chargingMoveName.erase(pokemon);
    runtimeMoveState.semiInvulnerableState.erase(pokemon);
    runtimeMoveState.typeShiftUsed.erase(pokemon);
    if (runtimeMoveState.pursuitSwitchTarget == pokemon) {
        runtimeMoveState.pursuitSwitchTarget = nullptr;
    }
}

void Battle::clearSideRuntimeState(const Side& side) {
    for (Pokemon* member : side.getTeam()) {
        clearPokemonRuntimeState(member);
    }
}

void Battle::tickEncoreForActor(Pokemon* actor) {
    auto it = runtimeMoveState.encoreState.find(actor);
    if (it == runtimeMoveState.encoreState.end()) {
        return;
    }
    it->second.remainingTurns -= 1;
    if (it->second.remainingTurns <= 0) {
        runtimeMoveState.encoreState.erase(it);
    }
}

Battle::SemiInvulnerableState Battle::getSemiInvulnerableState(const Pokemon* pokemon) const {
    auto it = runtimeMoveState.semiInvulnerableState.find(const_cast<Pokemon*>(pokemon));
    if (it == runtimeMoveState.semiInvulnerableState.end()) {
        return SemiInvulnerableState::None;
    }
    return it->second;
}

void Battle::beginTurn() {
    ++turnNumber;
    runtimeMoveState.roundUsedThisTurn = false;
    queue.setTrickRoom(field.isTrickRoom());

    applyStatusEffects();

    triggerAbilities(Trigger::OnTurnStart, nullptr);
    triggerItemEffects(ItemTrigger::OnTurnStart, nullptr);
}

void Battle::applyStatusEffects() {
    for (Side* side : {&sideA, &sideB}) {
        Pokemon* active = side->getActivePokemon();
        if (!active || active->isFainted()) {
            continue;
        }

        if (active->hasStatus(StatusType::Sleep)) {
            active->tickStatusDuration(StatusType::Sleep);
        }
        if (active->hasStatus(StatusType::Flinch)) {
            active->tickStatusDuration(StatusType::Flinch);
        }
    }
}

void Battle::resetActiveProtection() {
    Pokemon* activeA = sideA.getActivePokemon();
    if (activeA) {
        activeA->setIsProtected(false);
    }

    Pokemon* activeB = sideB.getActivePokemon();
    if (activeB) {
        activeB->setIsProtected(false);
    }
}

void Battle::endTurn() {
    triggerAbilities(Trigger::OnTurnEnd, nullptr);
    triggerItemEffects(ItemTrigger::OnTurnEnd, nullptr);

    applyWeatherEffects();
    applyFieldEffects();
    field.tick();
    weather.tick();

    resetActiveProtection();
}

Move Battle::applyEncoreOverride(Pokemon* actor, const Move& intendedMove) const {
    Move selectedMove = intendedMove;
    const auto encoreIt = runtimeMoveState.encoreState.find(actor);
    if (encoreIt == runtimeMoveState.encoreState.end() || encoreIt->second.lockedMoveName.empty()) {
        return selectedMove;
    }

    if (normalizeMoveName(selectedMove.getName()) == normalizeMoveName(encoreIt->second.lockedMoveName)) {
        return selectedMove;
    }

    const Move forcedMove = createMoveByName(encoreIt->second.lockedMoveName);
    if (!forcedMove.getName().empty()) {
        selectedMove = forcedMove;
    }
    return selectedMove;
}

bool Battle::handleTwoTurnChargeTurn(Pokemon* actor, const Move& selectedMove) {
    if (!isTwoTurnSemiInvulnerableMove(selectedMove)) {
        return false;
    }

    const std::string normalizedMove = normalizeMoveName(selectedMove.getName());
    const auto chargingIt = runtimeMoveState.chargingMoveName.find(actor);
    if (chargingIt != runtimeMoveState.chargingMoveName.end() && chargingIt->second == normalizedMove) {
        runtimeMoveState.chargingMoveName.erase(actor);
        runtimeMoveState.semiInvulnerableState.erase(actor);
        return false;
    }

    runtimeMoveState.chargingMoveName[actor] = normalizedMove;
    runtimeMoveState.semiInvulnerableState[actor] = stateForTwoTurnMove(selectedMove);
    recordExecutedMove(actor, selectedMove);
    return true;
}

void Battle::recordExecutedMove(Pokemon* actor, const Move& selectedMove) {
    runtimeMoveState.lastUsedMoveName[actor] = selectedMove.getName();
    if (moveNameEquals(selectedMove, "round")) {
        runtimeMoveState.roundUsedThisTurn = true;
    }
    tickEncoreForActor(actor);
}

void Battle::handlePursuitOnSwitch(Pokemon* switchingPokemon, Side* switchingSide) {
    if (!switchingPokemon || !switchingSide) {
        return;
    }

    Pokemon* opponentActive = (switchingSide == &sideA) ? sideB.getActivePokemon() : sideA.getActivePokemon();
    if (!opponentActive || switchingPokemon->isFainted() || opponentActive->isFainted()) {
        return;
    }

    for (const Move& candidate : opponentActive->getMoves()) {
        if (!moveNameEquals(candidate, "pursuit")) {
            continue;
        }

        runtimeMoveState.pursuitSwitchTarget = switchingPokemon;
        const int pursuitDamage = calculateDamage(opponentActive, switchingPokemon, candidate);
        runtimeMoveState.pursuitSwitchTarget = nullptr;
        switchingPokemon->setCurrentHP(switchingPokemon->getCurrentHP() - pursuitDamage);
        break;
    }
}

Side& Battle::getSideA() {
    return sideA;
}

const Side& Battle::getSideA() const {
    return sideA;
}

Side& Battle::getSideB() {
    return sideB;
}

const Side& Battle::getSideB() const {
    return sideB;
}

Side& Battle::getOpponentSide(const Side& side) {
    if (&side == &sideA) {
        return sideB;
    }
    return sideA;
}

Field& Battle::getField() {
    return field;
}

Weather& Battle::getWeather() {
    return weather;
}

EventSystem& Battle::getEventSystem() {
    return eventSystem;
}

const EventSystem& Battle::getEventSystem() const {
    return eventSystem;
}

void Battle::enqueueAction(const BattleAction& action) {
    BattleAction adjusted = action;
    if (adjusted.type == ActionType::Attack
        && adjusted.actor
        && adjusted.actor->getAbility() == AbilityType::Prankster
        && adjusted.move.getCategory() == Category::Status) {
        adjusted.movePriority += 1;
    }
    if (adjusted.type == ActionType::Attack && adjusted.actor && adjusted.actor->getItemType() == ItemType::QuickClaw) {
        if (PRNG::nextFloat(0.0f, 1.0f) < 0.2f) {
            adjusted.priority = std::numeric_limits<int>::max() / 4;
        }
    }
    queue.push(adjusted, field.isTrickRoom());
}

void Battle::processTurn() {
    beginTurn();
    
    // 输出回合开始的Json
    json turnStartJson = BattleToJson::battleToJson(*this);
    turnStartJson["event"] = "turn_start";
    
    // 写入到cache文件
    BattleToJson::writeToCache(turnStartJson, "battle_turn_start_" + std::to_string(turnNumber) + ".json");
    
    while (!queue.empty()) {
        resolveNextAction();
    }

    endTurn();
    
    // 输出回合结束的Json
    json turnEndJson = BattleToJson::battleToJson(*this);
    turnEndJson["event"] = "turn_end";
    
    // 写入到cache文件
    BattleToJson::writeToCache(turnEndJson, "battle_turn_end_" + std::to_string(turnNumber) + ".json");
    
}

void Battle::resolveNextAction() {
    BattleAction action = queue.pop();
    const bool canEmitActionEvents = action.actor != nullptr;
    ScopedActionEventEmitter eventScope{&eventSystem, turnNumber, &action, canEmitActionEvents};
    if (canEmitActionEvents) {
        eventSystem.emitActionEvent(RuntimeActionEvent{
            RuntimeEventPhase::BeforeActionResolve,
            turnNumber,
            action,
        });
    }

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

            if (action.actor->hasStatus(StatusType::Sleep)) {
                json attackResultJson = BattleToJson::battleToJson(*this);
                attackResultJson["event"] = "attack_result";
                attackResultJson["actor"] = action.actor->getName();
                attackResultJson["target"] = action.target->getName();
                attackResultJson["move"] = action.move.getName();
                attackResultJson["missed"] = true;
                BattleToJson::writeToCache(attackResultJson, "battle_attack_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + "_" + action.move.getName() + ".json");
                break;
            }

            if (action.actor->hasStatus(StatusType::Flinch)) {
                action.actor->tickStatusDuration(StatusType::Flinch);

                json attackResultJson = BattleToJson::battleToJson(*this);
                attackResultJson["event"] = "attack_result";
                attackResultJson["actor"] = action.actor->getName();
                attackResultJson["target"] = action.target->getName();
                attackResultJson["move"] = action.move.getName();
                attackResultJson["flinched"] = true;
                BattleToJson::writeToCache(attackResultJson, "battle_attack_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + "_" + action.move.getName() + ".json");
                break;
            }
            
            // 触发攻击前的效果
            triggerAbilities(Trigger::OnAttack, action.actor);
            triggerItemEffects(ItemTrigger::OnAttack, action.actor);
            
            // 计算伤害
            Move selectedMove = applyEncoreOverride(action.actor, action.move);
            if (handleTwoTurnChargeTurn(action.actor, selectedMove)) {
                return;
            }

            const AbilityType attackerAbility = action.actor->getAbility();
            if ((attackerAbility == AbilityType::Protean || attackerAbility == AbilityType::Libero)
                && !runtimeMoveState.typeShiftUsed[action.actor]) {
                action.actor->setTypes(selectedMove.getType(), Type::Count);
                runtimeMoveState.typeShiftUsed[action.actor] = true;
            }

            const bool ignoresSubstitute = attackerAbility == AbilityType::Infiltrator;
            const bool blockedBySubstitute = !ignoresSubstitute
                && action.target->getSubstituteHP() > 0
                && selectedMove.getCategory() == Category::Status
                && (selectedMove.getTarget() == Target::Opponent
                    || selectedMove.getTarget() == Target::AllOpponents
                    || selectedMove.getTarget() == Target::All);
            if (!blockedBySubstitute && !doesMoveHit(action.actor, action.target, selectedMove)) {
                applyCrashRecoil(action.actor, selectedMove);
                recordExecutedMove(action.actor, selectedMove);

                json attackResultJson = BattleToJson::battleToJson(*this);
                attackResultJson["event"] = "attack_result";
                attackResultJson["actor"] = action.actor->getName();
                attackResultJson["target"] = action.target->getName();
                attackResultJson["move"] = action.move.getName();
                BattleToJson::writeToCache(attackResultJson, "battle_attack_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + "_" + action.move.getName() + ".json");
                break;
            }

            if (blockedBySubstitute) {
                recordExecutedMove(action.actor, selectedMove);

                json attackResultJson = BattleToJson::battleToJson(*this);
                attackResultJson["event"] = "attack_result";
                attackResultJson["actor"] = action.actor->getName();
                attackResultJson["target"] = action.target->getName();
                attackResultJson["move"] = action.move.getName();
                BattleToJson::writeToCache(attackResultJson, "battle_attack_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + "_" + action.move.getName() + ".json");
                break;
            }

            const auto multiHitConfig = getMultiHitConfig(selectedMove);
            if (multiHitConfig.has_value()) {
                bool dealtDamage = false;
                int landedHits = 0;
                const int maxHits = rollHitCount(*multiHitConfig);

                for (int hitIndex = 0; hitIndex < maxHits; ++hitIndex) {
                    if (!doesMoveHit(action.actor, action.target, selectedMove)) {
                        if (multiHitConfig->stopOnMiss) {
                            break;
                        }
                        break;
                    }

                    ++landedHits;
                    int damage = calculateDamage(action.actor, action.target, selectedMove);
                    ItemDamageContext damageContext;
                    damageContext.move = &selectedMove;
                    damageContext.damage = damage;
                    damageContext.hpBeforeDamage = action.target->getCurrentHP();
                    damageContext.wasSuperEffective = action.target->getTypeEffectiveness(selectedMove.getType()) > 1.0f;
                    damageContext.isDamagingMove = true;
                    damageContext.isContact = true;

                    if (!ignoresSubstitute && action.target->getSubstituteHP() > 0 && damage > 0) {
                        const int remainingSubstituteHp = action.target->getSubstituteHP() - damage;
                        if (remainingSubstituteHp > 0) {
                            action.target->setSubstituteHP(remainingSubstituteHp);
                        } else {
                            action.target->clearSubstitute();
                        }
                        damage = 0;
                    }

                    action.target->setCurrentHP(action.target->getCurrentHP() - damage);

                    if (damage > 0) {
                        dealtDamage = true;
                        triggerAbilities(Trigger::OnDamage, action.target);
                        triggerItemEffect(action.target, ItemTrigger::OnDamage, action.actor, &damageContext);

                        triggerAbilities(Trigger::OnDealDamage, action.actor);
                        triggerItemEffect(action.actor, ItemTrigger::OnDealDamage, action.target, &damageContext);

                        if (action.target->isFainted()) {
                            if (action.actor->getAbility() == AbilityType::Moxie) {
                                action.actor->changeStatStage(StatIndex::Attack, 1);
                            }
                            Side* targetSide = findSideForPokemon(*this, action.target);
                            if (targetSide) {
                                targetSide->resetProtectCount();
                                if (targetSide->autoSwitchNext()) {
                                    applyEntryHazardsOnSwitchIn(targetSide, targetSide->getActivePokemon());
                                }
                            }
                            break;
                        }
                    }
                }

                if (landedHits > 0 && !(attackerAbility == AbilityType::SheerForce && isSheerForceBoostedMove(selectedMove))) {
                    processMoveEffects(action.actor, action.target, selectedMove);
                }
                recordExecutedMove(action.actor, selectedMove);

                json attackResultJson = BattleToJson::battleToJson(*this);
                attackResultJson["event"] = "attack_result";
                attackResultJson["actor"] = action.actor->getName();
                attackResultJson["target"] = action.target->getName();
                attackResultJson["move"] = selectedMove.getName();
                attackResultJson["hit_count"] = landedHits;
                attackResultJson["dealt_damage"] = dealtDamage;
                BattleToJson::writeToCache(attackResultJson, "battle_attack_result_" + std::to_string(turnNumber) + "_" + action.actor->getName() + "_" + selectedMove.getName() + ".json");
                break;
            }

            int damage = calculateDamage(action.actor, action.target, selectedMove);
            ItemDamageContext damageContext;
            damageContext.move = &selectedMove;
            damageContext.damage = damage;
            damageContext.hpBeforeDamage = action.target->getCurrentHP();
            damageContext.wasSuperEffective = action.target->getTypeEffectiveness(selectedMove.getType()) > 1.0f;
            damageContext.isDamagingMove = selectedMove.getCategory() != Category::Status;
            damageContext.isContact = selectedMove.getCategory() == Category::Physical;

            if (!ignoresSubstitute && action.target->getSubstituteHP() > 0 && damage > 0) {
                const int remainingSubstituteHp = action.target->getSubstituteHP() - damage;
                if (remainingSubstituteHp > 0) {
                    action.target->setSubstituteHP(remainingSubstituteHp);
                } else {
                    action.target->clearSubstitute();
                }
                damage = 0;
            }
            
            // 输出攻击信息
            // 应用伤害
            action.target->setCurrentHP(action.target->getCurrentHP() - damage);
            
            if (damage > 0) {
                // 触发受到伤害时的效果
                triggerAbilities(Trigger::OnDamage, action.target);
                triggerItemEffect(action.target, ItemTrigger::OnDamage, action.actor, &damageContext);

                // 检查目标是否濒死
                if (action.target->isFainted()) {
                    if (action.actor->getAbility() == AbilityType::Moxie) {
                        action.actor->changeStatStage(StatIndex::Attack, 1);
                    }
                    Side* targetSide = findSideForPokemon(*this, action.target);
                    if (targetSide) {
                        // 宝可梦死亡时重置保护计数
                        targetSide->resetProtectCount();
                        if (targetSide->autoSwitchNext()) {
                            applyEntryHazardsOnSwitchIn(targetSide, targetSide->getActivePokemon());
                        }
                    }
                }
            }
            
            // 处理技能追加效果
            if (!(attackerAbility == AbilityType::SheerForce && isSheerForceBoostedMove(selectedMove))) {
                processMoveEffects(action.actor, action.target, selectedMove);
            }
            
            // 触发攻击后的效果
            if (damage > 0) {
                triggerAbilities(Trigger::OnDealDamage, action.actor);
                triggerItemEffect(action.actor, ItemTrigger::OnDealDamage, action.target, &damageContext);
            }
            recordExecutedMove(action.actor, selectedMove);
            
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
                Pokemon* oldPokemon = action.actor;
                handlePursuitOnSwitch(oldPokemon, side);

                // 触发退场时的效果
                triggerAbilities(Trigger::OnExit, action.actor);
                triggerItemEffects(ItemTrigger::OnSwitchOut, action.actor);
                
                // 执行切换
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
                const Side* actorSide = findSideForPokemon(*this, action.actor);
                Pokemon* opponentActive = nullptr;
                if (actorSide == &sideA) {
                    opponentActive = sideB.getActivePokemon();
                } else if (actorSide == &sideB) {
                    opponentActive = sideA.getActivePokemon();
                }

                if (isBerryItem(action.item)
                    && opponentActive
                    && !opponentActive->isFainted()
                    && opponentActive->getAbility() == AbilityType::Unnerve) {
                    break;
                }

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

    // 幽灵突袭类可无视保护。
    if (defender->getIsProtected() && !moveNameEquals(move, "phantomforce") && !moveNameEquals(move, "shadowforce")) {
        return 0;
    }

    const AbilityType atkAbility = attacker->getAbility();
    const AbilityType defAbility = defender->getAbility();
    const Pokemon* activeA = sideA.getActivePokemon();
    const Pokemon* activeB = sideB.getActivePokemon();
    const bool beadsOfRuinActive = (activeA && !activeA->isFainted() && activeA->getAbility() == AbilityType::BeadsOfRuin)
        || (activeB && !activeB->isFainted() && activeB->getAbility() == AbilityType::BeadsOfRuin);
    const bool swordOfRuinActive = (activeA && !activeA->isFainted() && activeA->getAbility() == AbilityType::SwordOfRuin)
        || (activeB && !activeB->isFainted() && activeB->getAbility() == AbilityType::SwordOfRuin);
    const bool tabletsOfRuinActive = (activeA && !activeA->isFainted() && activeA->getAbility() == AbilityType::TabletsOfRuin)
        || (activeB && !activeB->isFainted() && activeB->getAbility() == AbilityType::TabletsOfRuin);
    const bool vesselOfRuinActive = (activeA && !activeA->isFainted() && activeA->getAbility() == AbilityType::VesselOfRuin)
        || (activeB && !activeB->isFainted() && activeB->getAbility() == AbilityType::VesselOfRuin);
    const bool weatherSuppressed = (activeA && !activeA->isFainted() && activeA->getAbility() == AbilityType::CloudNine)
        || (activeB && !activeB->isFainted() && activeB->getAbility() == AbilityType::CloudNine);

    const SemiInvulnerableState defenderState = getSemiInvulnerableState(defender);
    if (defenderState != SemiInvulnerableState::None) {
        bool canHitSemiInvulnerableTarget = false;
        switch (defenderState) {
            case SemiInvulnerableState::Underground:
                canHitSemiInvulnerableTarget = moveNameEquals(move, "earthquake") || moveNameEquals(move, "magnitude");
                break;
            case SemiInvulnerableState::Airborne:
                canHitSemiInvulnerableTarget = moveNameEquals(move, "gust") || moveNameEquals(move, "twister")
                    || moveNameEquals(move, "thunder") || moveNameEquals(move, "hurricane");
                break;
            case SemiInvulnerableState::Underwater:
                canHitSemiInvulnerableTarget = moveNameEquals(move, "surf") || moveNameEquals(move, "whirlpool");
                break;
            case SemiInvulnerableState::Phased:
                canHitSemiInvulnerableTarget = false;
                break;
            case SemiInvulnerableState::None:
                canHitSemiInvulnerableTarget = true;
                break;
        }

        if (!canHitSemiInvulnerableTarget) {
            return 0;
        }
    }

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
    if (defAbility == AbilityType::SapSipper && move.getType() == Type::Grass) {
        defender->changeStatStage(StatIndex::Attack, 1);
        return 0;
    }
    if (defAbility == AbilityType::StormDrain && move.getType() == Type::Water) {
        defender->changeStatStage(StatIndex::SpecialAttack, 1);
        return 0;
    }
    if (defAbility == AbilityType::MotorDrive && move.getType() == Type::Electric) {
        defender->changeStatStage(StatIndex::Speed, 1);
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
        const int atkStage = (defAbility == AbilityType::Unaware) ? 0 : attacker->getStatStage(StatIndex::Attack);
        const int defStage = (atkAbility == AbilityType::Unaware) ? 0 : defender->getStatStage(StatIndex::Defense);
        attackStat *= stageMultiplier(atkStage);
        defenseStat *= stageMultiplier(defStage);
        if (tabletsOfRuinActive && atkAbility != AbilityType::TabletsOfRuin) {
            attackStat *= 0.75f;
        }
        if (swordOfRuinActive && defAbility != AbilityType::SwordOfRuin) {
            defenseStat *= 0.75f;
        }
    } else if (move.getCategory() == Category::Special) {
        const int spaStage = (defAbility == AbilityType::Unaware) ? 0 : attacker->getStatStage(StatIndex::SpecialAttack);
        const int spdStage = (atkAbility == AbilityType::Unaware) ? 0 : defender->getStatStage(StatIndex::SpecialDefense);
        attackStat *= stageMultiplier(spaStage);
        defenseStat *= stageMultiplier(spdStage);
        if (vesselOfRuinActive && atkAbility != AbilityType::VesselOfRuin) {
            attackStat *= 0.75f;
        }
        if (beadsOfRuinActive && defAbility != AbilityType::BeadsOfRuin) {
            defenseStat *= 0.75f;
        }
    }

    if (atkAbility == AbilityType::HugePower && move.getCategory() == Category::Physical) {
        attackStat *= 2.0f;
    }
    if (atkAbility == AbilityType::Guts && move.getCategory() == Category::Physical && hasMajorStatus(attacker)) {
        attackStat *= 1.5f;
    }
    if (atkAbility == AbilityType::HadronEngine && field.isActive() && field.type == FieldType::Electric
        && move.getCategory() == Category::Special) {
        attackStat *= 1.3f;
    }
    if (defAbility == AbilityType::MarvelScale && hasMajorStatus(defender)) {
        defenseStat *= 1.5f;
    }

    if (defenseStat <= 0.0f) {
        defenseStat = 1.0f;
    }

    Type effectiveType = move.getType();
    int movePower = move.getPower();
    if (moveNameEquals(move, "heavyslam")) {
        movePower = heavySlamPowerByWeightRatio(attacker->getSpecies().weight, defender->getSpecies().weight);
    }
    if (moveNameEquals(move, "weatherball")) {
        effectiveType = weatherSuppressed ? Type::Normal : weatherBallType(weather);
        if (!weatherSuppressed && weather.isActive()) {
            movePower *= 2;
        }
    }
    if (moveNameEquals(move, "round") && runtimeMoveState.roundUsedThisTurn) {
        movePower *= 2;
    }

    if (atkAbility == AbilityType::Technician && move.getCategory() != Category::Status && movePower <= 60) {
        movePower = static_cast<int>(std::lround(movePower * 1.5f));
    }
    if (atkAbility == AbilityType::SheerForce && isSheerForceBoostedMove(move)) {
        movePower = static_cast<int>(std::lround(movePower * 1.3f));
    }

    float base = ((2.0f * attacker->getLevel() / 5.0f + 2.0f) * movePower * attackStat / defenseStat) / 50.0f + 2.0f;
    const float typeEffectiveness = defender->getTypeEffectiveness(effectiveType);
    float modifier = typeEffectiveness;
    
    // 应用天气加成并输出日志
    float weatherModifier = weatherSuppressed ? 1.0f : weather.applyDamageModifier(effectiveType);
    modifier *= weatherModifier;
    
    float damage = base * modifier;

    const bool isStab = effectiveType == attacker->getType1() || effectiveType == attacker->getType2();
    if (isStab) {
        damage *= (atkAbility == AbilityType::Adaptability) ? 2.0f : 1.5f;
    }

    if (defAbility == AbilityType::ThickFat && (move.getType() == Type::Fire || move.getType() == Type::Ice)) {
        damage *= 0.5f;
    }
    if ((defAbility == AbilityType::Filter || defAbility == AbilityType::SolidRock) && typeEffectiveness > 1.0f) {
        damage *= 0.75f;
    }

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
    if (moveNameEquals(move, "knockoff") && defender->getItemType() != ItemType::None) {
        damage *= 1.5f;
    }
    if (moveNameEquals(move, "pursuit") && defender == runtimeMoveState.pursuitSwitchTarget) {
        damage *= 2.0f;
    }
    if (defenderState == SemiInvulnerableState::Underground
        && (moveNameEquals(move, "earthquake") || moveNameEquals(move, "magnitude"))) {
        damage *= 2.0f;
    }
    if (defenderState == SemiInvulnerableState::Airborne
        && (moveNameEquals(move, "gust") || moveNameEquals(move, "twister"))) {
        damage *= 2.0f;
    }
    if (defenderState == SemiInvulnerableState::Underwater
        && (moveNameEquals(move, "surf") || moveNameEquals(move, "whirlpool"))) {
        damage *= 2.0f;
    }

    const FieldType fieldType = field.type;
    switch (fieldType) {
        case FieldType::Psychic:
            if (move.getCategory() == Category::Physical) {
                damage *= 0.5f;
            }
            break;
        case FieldType::Electric:
            if (move.getType() == Type::Electric) {
                damage *= 1.5f;
            }
            break;
        case FieldType::Misty:
            if (move.getType() == Type::Dragon) {
                damage *= 0.5f;
            }
            break;
        default:
            break;
    }

    const Side* defenderSide = findSideForPokemon(*this, defender);
    if (defenderSide) {
        if (defenderSide->hasReflect() && move.getCategory() == Category::Physical && atkAbility != AbilityType::Infiltrator) {
            damage *= 0.5f;
        }
        if (defenderSide->hasLightScreen() && move.getCategory() == Category::Special && atkAbility != AbilityType::Infiltrator) {
            damage *= 0.5f;
        }
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
    Pokemon* oldActive = side.getActivePokemon();
    bool switched = side.switchActive(newIndex);
    if (switched) {
        if (oldActive) {
            oldActive->resetTypesToSpecies();
        }
        if (oldActive && !oldActive->isFainted()) {
            if (oldActive->getAbility() == AbilityType::Regenerator) {
                const int heal = std::max(1, oldActive->getMaxHP() / 3);
                oldActive->setCurrentHP(oldActive->getCurrentHP() + heal);
            }
            if (oldActive->getAbility() == AbilityType::NaturalCure) {
                oldActive->clearStatuses();
            }
        }

        Pokemon* newActive = side.getActivePokemon();
        applyEntryHazardsOnSwitchIn(&side, newActive);
        clearPokemonRuntimeState(oldActive);
        clearVolatileSwitchState(oldActive);
        // 切换宝可梦时重置保护计数
        side.resetProtectCount();
    }
    return switched;
}

void Battle::applyEntryHazardsOnSwitchIn(Side* enteringSide, Pokemon* enteringPokemon) {
    if (!enteringSide || !enteringPokemon || enteringPokemon->isFainted()) {
        return;
    }

    if (enteringSide->hasStealthRock()) {
        const float effectiveness = enteringPokemon->getTypeEffectiveness(Type::Rock);
        const int stealthRockDamage = std::max(1, static_cast<int>(std::lround(enteringPokemon->getMaxHP() * 0.125f * effectiveness)));
        enteringPokemon->setCurrentHP(enteringPokemon->getCurrentHP() - stealthRockDamage);
    }

    const bool isFlyingType = enteringPokemon->getType1() == Type::Flying || enteringPokemon->getType2() == Type::Flying;
    const bool hasLevitate = enteringPokemon->getAbility() == AbilityType::Levitate;
    const bool grounded = !isFlyingType && !hasLevitate;

    if (grounded && enteringSide->getSpikesLayers() > 0) {
        float spikesFactor = 0.125f;
        if (enteringSide->getSpikesLayers() == 2) {
            spikesFactor = 1.0f / 6.0f;
        } else if (enteringSide->getSpikesLayers() >= 3) {
            spikesFactor = 0.25f;
        }
        const int spikesDamage = std::max(1, static_cast<int>(std::lround(enteringPokemon->getMaxHP() * spikesFactor)));
        enteringPokemon->setCurrentHP(enteringPokemon->getCurrentHP() - spikesDamage);
    }

    if (grounded && enteringSide->getToxicSpikesLayers() > 0) {
        const bool poisonType = enteringPokemon->getType1() == Type::Poison || enteringPokemon->getType2() == Type::Poison;
        const bool steelType = enteringPokemon->getType1() == Type::Steel || enteringPokemon->getType2() == Type::Steel;
        if (!poisonType && !steelType) {
            if (enteringSide->getToxicSpikesLayers() >= 2) {
                enteringPokemon->addStatus(StatusType::ToxicPoison);
            } else {
                enteringPokemon->addStatus(StatusType::Poison);
            }
        }
    }
}

void Battle::processMoveEffects(Pokemon* attacker, Pokemon* defender, const Move& move) {
    if (!attacker || !defender) return;

    auto applyBatonPassSwitch = [&](Pokemon* source) {
        Side* actorSide = findSideForPokemon(*this, source);
        if (!actorSide || !actorSide->canSwitch()) {
            return;
        }

        const int atkStage = source->getStatStage(StatIndex::Attack);
        const int defStage = source->getStatStage(StatIndex::Defense);
        const int spaStage = source->getStatStage(StatIndex::SpecialAttack);
        const int spdStage = source->getStatStage(StatIndex::SpecialDefense);
        const int speStage = source->getStatStage(StatIndex::Speed);

        if (!actorSide->autoSwitchNext()) {
            return;
        }

        Pokemon* newActive = actorSide->getActivePokemon();
        if (!newActive || newActive == source) {
            return;
        }

        const int newAtk = newActive->getStatStage(StatIndex::Attack);
        const int newDef = newActive->getStatStage(StatIndex::Defense);
        const int newSpa = newActive->getStatStage(StatIndex::SpecialAttack);
        const int newSpd = newActive->getStatStage(StatIndex::SpecialDefense);
        const int newSpe = newActive->getStatStage(StatIndex::Speed);

        newActive->changeStatStage(StatIndex::Attack, atkStage - newAtk);
        newActive->changeStatStage(StatIndex::Defense, defStage - newDef);
        newActive->changeStatStage(StatIndex::SpecialAttack, spaStage - newSpa);
        newActive->changeStatStage(StatIndex::SpecialDefense, spdStage - newSpd);
        newActive->changeStatStage(StatIndex::Speed, speStage - newSpe);

        Pokemon* opponentPokemon = (actorSide == &sideA) ? sideB.getActivePokemon() : sideA.getActivePokemon();
        applyEntryHazardsOnSwitchIn(actorSide, newActive);
        triggerAbility(newActive, Trigger::OnEntry, opponentPokemon);
        triggerItemEffect(newActive, ItemTrigger::OnEntry, opponentPokemon);
    };

    if (moveRuleRegistry.apply(*this, attacker, defender, move)) {
        return;
    }

    if (moveNameEquals(move, "encore")) {
        const auto it = runtimeMoveState.lastUsedMoveName.find(defender);
        if (it != runtimeMoveState.lastUsedMoveName.end() && !it->second.empty()) {
            runtimeMoveState.encoreState[defender] = Battle::EncoreState{it->second, 3};
        }
        return;
    }

    if (moveNameEquals(move, "knockoff")) {
        if (defender->getItemType() != ItemType::None) {
            defender->removeItem();
        }
        return;
    }

    if (moveNameEquals(move, "batonpass")) {
        applyBatonPassSwitch(attacker);
        return;
    }

    if (moveNameEquals(move, "uturn")) {
        if (!attacker->isFainted()) {
            Side* actorSide = findSideForPokemon(*this, attacker);
            if (actorSide && actorSide->canSwitch()) {
                actorSide->autoSwitchNext();
                Pokemon* newActive = actorSide->getActivePokemon();
                Pokemon* opponentPokemon = (actorSide == &sideA) ? sideB.getActivePokemon() : sideA.getActivePokemon();
                if (newActive && newActive != attacker) {
                    applyEntryHazardsOnSwitchIn(actorSide, newActive);
                    triggerAbility(newActive, Trigger::OnEntry, opponentPokemon);
                    triggerItemEffect(newActive, ItemTrigger::OnEntry, opponentPokemon);
                }
            }
        }
        return;
    }

    if (field.isActive() && field.type == FieldType::Misty && isStatusInflictingEffect(move.getEffect())) {
        return;
    }
    
    MoveEffectHandlers::applyStandardMoveEffect(*this, attacker, defender, move);
}

void Battle::applyWeatherEffects() {
    if (!weather.isActive()) {
        return;
    }

    Pokemon* activeA = sideA.getActivePokemon();
    Pokemon* activeB = sideB.getActivePokemon();
    const bool weatherSuppressed = (activeA && !activeA->isFainted() && activeA->getAbility() == AbilityType::CloudNine)
        || (activeB && !activeB->isFainted() && activeB->getAbility() == AbilityType::CloudNine);
    if (weatherSuppressed) {
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
                if (active->getAbility() == AbilityType::MagicGuard) {
                    break;
                }
                if (active->getType1() != Type::Rock && active->getType1() != Type::Ground && active->getType1() != Type::Steel && 
                    active->getType2() != Type::Rock && active->getType2() != Type::Ground && active->getType2() != Type::Steel) {
                    int damage = std::max(1, active->getMaxHP() / 16);
                    active->setCurrentHP(active->getCurrentHP() - damage);
                }
                break;
            case WeatherType::Hail:
                // 冰雹：对非冰属性的宝可梦造成伤害
                if (active->getAbility() == AbilityType::MagicGuard) {
                    break;
                }
                if (active->getType1() != Type::Ice && active->getType2() != Type::Ice) {
                    int damage = std::max(1, active->getMaxHP() / 16);
                    active->setCurrentHP(active->getCurrentHP() - damage);
                }
                break;
            case WeatherType::Snow:
                // 雪：对冰属性以外的宝可梦造成少量伤害
                if (active->getAbility() == AbilityType::MagicGuard) {
                    break;
                }
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
    for (Side* side : {&sideA, &sideB}) {
        Pokemon* active = side->getActivePokemon();
        if (!active || active->isFainted()) {
            side->tickScreenEffects();
            continue;
        }

        Pokemon* seedSource = active->getLeechSeedSource();
        if (seedSource && !seedSource->isFainted()) {
            const int drain = std::max(1, active->getMaxHP() / 8);
            active->setCurrentHP(active->getCurrentHP() - drain);
            seedSource->setCurrentHP(seedSource->getCurrentHP() + drain);
        }

        side->tickScreenEffects();
    }

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

    if (trigger == Trigger::OnEntry) {
        switch (abilityType) {
            case AbilityType::Drizzle:
                weather.setWeather(WeatherType::Rain, 5);
                break;
            case AbilityType::Drought:
                weather.setWeather(WeatherType::Sun, 5);
                break;
            case AbilityType::SandStream:
                weather.setWeather(WeatherType::Sandstorm, 5);
                break;
            case AbilityType::SnowWarning:
                weather.setWeather(WeatherType::Snow, 5);
                break;
            case AbilityType::GrassySurge:
                field.setField(FieldType::Grassy, 5);
                break;
            case AbilityType::ElectricSurge:
                field.setField(FieldType::Electric, 5);
                break;
            case AbilityType::PsychicSurge:
                field.setField(FieldType::Psychic, 5);
                break;
            case AbilityType::MistySurge:
                field.setField(FieldType::Misty, 5);
                break;
            case AbilityType::HadronEngine:
                field.setField(FieldType::Electric, 5);
                break;
            default:
                break;
        }
    }
    
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
