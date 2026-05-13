#include "battle/Moves.h"
#include "battle/GameRegistry.h"

#include "battle/Battle.h"
#include "battle/PRNG.h"
#include "battle/Utils.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

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

bool isSelfStatDropMove(const Move& move) {
    return moveNameEquals(move, "closecombat") || moveNameEquals(move, "superpower")
        || moveNameEquals(move, "overheat") || moveNameEquals(move, "leafstorm")
        || moveNameEquals(move, "dracometeor") || moveNameEquals(move, "fleurcannon")
        || moveNameEquals(move, "steelbeam") || moveNameEquals(move, "vcreate")
        || moveNameEquals(move, "clangoroussoulblaze");
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
        attacker->changeStatStage(StatIndex::Attack, 1);
        attacker->changeStatStage(StatIndex::Defense, 1);
        attacker->changeStatStage(StatIndex::SpecialAttack, 1);
        attacker->changeStatStage(StatIndex::SpecialDefense, 1);
        attacker->changeStatStage(StatIndex::Speed, 1);
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

int firstAvailableSwitchIndex(const Side& side) {
    const int activeIndex = side.getActiveIndex();
    const auto& team = side.getTeam();
    const int teamSize = side.getPokemonCount();

    for (int i = 0; i < teamSize; ++i) {
        if (i == activeIndex) {
            continue;
        }
        if (i >= 0 && i < static_cast<int>(team.size()) && team[i] && !team[i]->isFainted()) {
            return i;
        }
    }

    return -1;
}

void triggerEntryEffectsAfterSwitch(BattleContext& ctx, Side& side) {
    Pokemon* newActive = side.getActivePokemon();
    if (!newActive) {
        return;
    }

    Pokemon* opponentPokemon = (&side == &ctx.getSideA()) ? ctx.getSideB().getActivePokemon() : ctx.getSideA().getActivePokemon();
    ctx.triggerAbility(newActive, Trigger::OnEntry, opponentPokemon);
    ctx.triggerItemEffect(newActive, ItemTrigger::OnEntry, opponentPokemon);
}

bool forceSwitchSide(BattleContext& ctx, Side& side) {
    const int nextIndex = firstAvailableSwitchIndex(side);
    if (nextIndex < 0) {
        return false;
    }

    if (!ctx.switchPokemon(side, nextIndex)) {
        return false;
    }

    triggerEntryEffectsAfterSwitch(ctx, side);
    return true;
}

bool canForceSwitchTarget(BattleContext& ctx, Pokemon* defender) {
    return ctx.canBeForcedToSwitch(defender);
}

bool applyConfusionMove(BattleContext& ctx, Pokemon* defender, const Move& move) {
    if (!defender || defender->getSubstituteHP() > 0 || defender->hasStatus(StatusType::Confusion)) {
        return true;
    }

    defender->addStatus(StatusType::Confusion);
    // 这里直接记录状态变化，避免把行为留在 Battle.cpp
    (void)ctx;
    return true;
}

Type findResistingTypeForAttack(Type attackType) {
    for (int candidate = 0; candidate < static_cast<int>(Type::Count); ++candidate) {
        const Type candidateType = static_cast<Type>(candidate);
        if (candidateType == Type::Count) {
            continue;
        }
        const float effectiveness = Utils::TypeEffectiveness[static_cast<int>(attackType)][candidate];
        if (effectiveness <= 0.5f) {
            return candidateType;
        }
    }
    return Type::Normal;
}
}  // namespace

Move::Move()
    : data{0, "", "", "", Type::Normal, Category::Status, 0, 100, 0, MoveEffect::None, 0, 0, 0, 0, Target::Opponent}, maxPP(0) {}

Move::Move(const MoveData& data)
    : data(data), maxPP(data.pp) {}

Move::Move(std::string name, Type type, Category category, int power, int accuracy, int pp)
    : data{0, std::move(name), "", "", type, category, power, accuracy, pp, MoveEffect::None, 0, 0, 0, 0, Target::Opponent}, maxPP(pp) {}

Move::Move(std::string name, Type type, Category category, int power, int accuracy, int pp, MoveEffect effect, int effectChance, int effectParam1, int effectParam2, int priority, Target target)
    : data{0, std::move(name), "", "", type, category, power, accuracy, pp, effect, effectChance, effectParam1, effectParam2, priority, target}, maxPP(pp) {}

Move::Move(const Move& other)
    : data(other.data), maxPP(other.maxPP) {}

Move& Move::operator=(const Move& other) {
    if (this != &other) {
        data = other.data;
        maxPP = other.maxPP;
    }
    return *this;
}

Move::~Move() {}

void initializeCoreMoveRules(GameRegistry& registry) {

    auto registerProtectLikeRule = [&registry](const std::string& name, const std::string& variant = std::string()) {
        registry.registerMoveRule(name, [variant](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
            Side* actorSide = ctx.findSideForPokemon( attacker);
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
                if (!variant.empty()) {
                    ctx.getRuntimeMoveState().protectionVariant[attacker] = variant;
                }
            } else {
                actorSide->resetProtectCount();
                ctx.getRuntimeMoveState().protectionVariant.erase(attacker);
            }
            return true;
        });
    };

    auto registerSideGuardLikeRule = [&registry](const std::string& name, bool quickGuard) {
        registry.registerMoveRule(name, [quickGuard](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
            Side* actorSide = ctx.findSideForPokemon( attacker);
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
                actorSide->setProtectCount(protectCount + 1);
                if (quickGuard) {
                    ctx.getRuntimeMoveState().quickGuardActive[actorSide] = true;
                } else {
                    ctx.getRuntimeMoveState().wideGuardActive[actorSide] = true;
                }
            } else {
                actorSide->resetProtectCount();
            }
            return true;
        });
    };

    auto registerRecoverLikeRule = [&registry](const std::string& name) {
        registry.registerMoveRule(name, [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
            const int healAmount = std::max(1, attacker->getMaxHP() / 2);
            attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
            return true;
        });
    };

    auto registerEvasionDropRule = [&registry](const std::string& name) {
        registry.registerMoveRule(name, [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
            if (defender->getSubstituteHP() > 0) {
                return true;
            }

            Side* defenderSide = ctx.findSideForPokemon( defender);
            if (defenderSide && defenderSide->hasMist()) {
                return true;
            }

            const AbilityType defenderAbility = defender->getAbility();
            if (abilityBlocksEvasionDrops(defenderAbility)) {
                return true;
            }
            if (abilityReflectsStatDrops(defenderAbility) && attacker) {
                attacker->changeEvasionStage(1);
                return true;
            }

            defender->changeEvasionStage(1);
            return true;
        });
    };

    auto registerAccuracyDropRule = [&registry](const std::string& name, int stages) {
        registry.registerMoveRule(name, [stages](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
            if (!defender || stages <= 0) {
                return true;
            }
            if (defender->getSubstituteHP() > 0) {
                return true;
            }

            Side* defenderSide = ctx.findSideForPokemon( defender);
            if (defenderSide && defenderSide->hasMist()) {
                return true;
            }

            const AbilityType defenderAbility = defender->getAbility();
            if (abilityBlocksAccuracyDrops(defenderAbility)) {
                return true;
            }
            if (abilityReflectsStatDrops(defenderAbility) && attacker) {
                attacker->changeAccuracyStage(-stages);
                return true;
            }

            defender->changeAccuracyStage(-stages);
            return true;
        });
    };

    registry.registerMoveRule("trickroom", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        if (ctx.getField().isTrickRoom()) {
            ctx.getField().setField(FieldType::None, 0);
        } else {
            ctx.getField().setField(FieldType::TrickRoom, 5);
        }
        return true;
    });

    registry.registerMoveRule("gravity", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getRuntimeMoveState().gravityTurns = 5;
        return true;
    });

    registry.registerMoveRule("raindance", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getWeather().setWeather(WeatherType::Rain, 5);
        return true;
    });
    registry.registerMoveRule("sunnyday", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getWeather().setWeather(WeatherType::Sun, 5);
        return true;
    });
    registry.registerMoveRule("sandstorm", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getWeather().setWeather(WeatherType::Sandstorm, 5);
        return true;
    });
    registry.registerMoveRule("hail", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getWeather().setWeather(WeatherType::Hail, 5);
        return true;
    });
    registry.registerMoveRule("snowscape", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getWeather().setWeather(WeatherType::Hail, 5);
        return true;
    });

    registry.registerMoveRule("psychicterrain", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getField().setField(FieldType::Psychic, 5);
        return true;
    });
    registry.registerMoveRule("electricterrain", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getField().setField(FieldType::Electric, 5);
        return true;
    });
    registry.registerMoveRule("grassyterrain", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getField().setField(FieldType::Grassy, 5);
        return true;
    });
    registry.registerMoveRule("mistyterrain", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        ctx.getField().setField(FieldType::Misty, 5);
        return true;
    });

    registry.registerMoveRule("mist", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setMistTurns(5);
        }
        return true;
    });

    registry.registerMoveRule("safeguard", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setSafeguardTurns(5);
        }
        return true;
    });

    registry.registerMoveRule("mudsport", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setMudSportTurns(5);
        }
        return true;
    });

    registry.registerMoveRule("watersport", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setWaterSportTurns(5);
        }
        return true;
    });

    registerProtectLikeRule("protect");
    registerProtectLikeRule("detect");
    registerProtectLikeRule("spikyshield", "spikyshield");
    registerProtectLikeRule("kingsshield", "kingsshield");
    registerProtectLikeRule("obstruct", "obstruct");
    registerProtectLikeRule("banefulbunker", "banefulbunker");
    registerProtectLikeRule("burningbulwark", "burningbulwark");
    registerSideGuardLikeRule("quickguard", true);
    registerSideGuardLikeRule("wideguard", false);

    registerRecoverLikeRule("recover");
    registerRecoverLikeRule("softboiled");
    registerRecoverLikeRule("milkdrink");
    registerRecoverLikeRule("slackoff");

    registry.registerMoveRule("shoreup", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        int healAmount = std::max(1, attacker->getMaxHP() / 2);
        if (ctx.getWeather().type == WeatherType::Sandstorm) {
            healAmount = std::max(1, (attacker->getMaxHP() * 2) / 3);
        }
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        return true;
    });

    registry.registerMoveRule("healpulse", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender || defender->isFainted()) {
            return true;
        }
        const int healAmount = std::max(1, defender->getMaxHP() / 2);
        defender->setCurrentHP(defender->getCurrentHP() + healAmount);
        return true;
    });

    registry.registerMoveRule("lifedew", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker || attacker->isFainted()) {
            return true;
        }
        const int healAmount = std::max(1, attacker->getMaxHP() / 4);
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        return true;
    });

    registry.registerMoveRule("lunarblessing", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker || attacker->isFainted()) {
            return true;
        }
        const int healAmount = std::max(1, attacker->getMaxHP() / 4);
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        attacker->clearStatuses();
        return true;
    });

    registry.registerMoveRule("morningsun", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        const int healAmount = weatherRecoveryAmount(ctx.getWeather(), attacker);
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        return true;
    });
    registry.registerMoveRule("synthesis", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        const int healAmount = weatherRecoveryAmount(ctx.getWeather(), attacker);
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        return true;
    });
    registry.registerMoveRule("moonlight", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        const int healAmount = weatherRecoveryAmount(ctx.getWeather(), attacker);
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        return true;
    });

    registry.registerMoveRule("focusenergy", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        int& stage = ctx.getRuntimeMoveState().criticalHitStage[attacker];
        stage = std::min(4, stage + 2);
        return true;
    });

    registry.registerMoveRule("rest", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }

        if (resolveStatusImmunity(attacker->getAbility(), StatusType::Sleep)) {
            return true;
        }

        attacker->setCurrentHP(attacker->getMaxHP());
        attacker->removeStatus(StatusType::Paralysis);
        attacker->removeStatus(StatusType::Burn);
        attacker->removeStatus(StatusType::Poison);
        attacker->removeStatus(StatusType::ToxicPoison);
        attacker->removeStatus(StatusType::Freeze);
        attacker->removeStatus(StatusType::Sleep);
        attacker->addStatus(StatusType::Sleep, 2);
        return true;
    });

    registry.registerMoveRule("wish", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) {
            return true;
        }
        ctx.getRuntimeMoveState().wishState[actorSide] = WishState{2, std::max(1, attacker->getMaxHP() / 2)};
        return true;
    });

    registry.registerMoveRule("healingwish", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) {
            return true;
        }
        ctx.getRuntimeMoveState().switchInRecoveryState[actorSide] = SwitchInRecoveryState{false};
        attacker->setCurrentHP(0);
        return true;
    });

    registry.registerMoveRule("lunardance", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) {
            return true;
        }
        ctx.getRuntimeMoveState().switchInRecoveryState[actorSide] = SwitchInRecoveryState{true};
        attacker->setCurrentHP(0);
        return true;
    });

    registry.registerMoveRule("metronome", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) {
            return true;
        }

        Move rolledMove;
        for (int tries = 0; tries < 32; ++tries) {
            const int randomMoveId = PRNG::nextInt(1, 938);
            Move candidate = createMoveById(randomMoveId);
            if (candidate.getName().empty()) {
                continue;
            }
            if (moveNameEquals(candidate, "metronome") || moveNameEquals(candidate, "mirrormove")) {
                continue;
            }
            rolledMove = candidate;
            break;
        }

        if (rolledMove.getName().empty()) {
            return true;
        }

        if (rolledMove.getCategory() != Category::Status) {
            const int damage = ctx.calculateDamage(attacker, defender, rolledMove);
            defender->setCurrentHP(defender->getCurrentHP() - damage);
        }

        ctx.processMoveEffects(attacker, defender, rolledMove);
        return true;
    });

    registry.registerMoveRule("mirrormove", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) {
            return true;
        }

        const auto copiedIt = ctx.getRuntimeMoveState().lastUsedMoveName.find(defender);
        if (copiedIt == ctx.getRuntimeMoveState().lastUsedMoveName.end() || copiedIt->second.empty()) {
            return true;
        }

        Move copiedMove = createMoveByName(copiedIt->second);
        if (copiedMove.getName().empty()) {
            return true;
        }
        if (moveNameEquals(copiedMove, "mirrormove") || moveNameEquals(copiedMove, "metronome")) {
            return true;
        }

        if (copiedMove.getCategory() != Category::Status) {
            const int damage = ctx.calculateDamage(attacker, defender, copiedMove);
            defender->setCurrentHP(defender->getCurrentHP() - damage);
        }

        ctx.processMoveEffects(attacker, defender, copiedMove);
        return true;
    });

    registry.registerMoveRule("trick", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || attacker->getSubstituteHP() > 0 || defender->getSubstituteHP() > 0) {
            return true;
        }

        const ItemType attackerItem = attacker->getItemType();
        const ItemType defenderItem = defender->getItemType();
        attacker->setItemType(defenderItem);
        defender->setItemType(attackerItem);
        return true;
    });

    registry.registerMoveRule("roleplay", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }

        const AbilityType copied = defender->getAbility();
        if (copied == AbilityType::None) {
            return true;
        }

        attacker->setAbility(copied);
        return true;
    });

    registry.registerMoveRule("skillswap", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }

        const AbilityType attackerAbility = attacker->getAbility();
        const AbilityType defenderAbility = defender->getAbility();
        attacker->setAbility(defenderAbility);
        defender->setAbility(attackerAbility);
        return true;
    });

    registry.registerMoveRule("foresight", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().foresightMarked[defender] = true;
        return true;
    });

    registry.registerMoveRule("odorsleuth", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().foresightMarked[defender] = true;
        return true;
    });

    registry.registerMoveRule("miracleeye", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().miracleEyeMarked[defender] = true;
        return true;
    });

    registry.registerMoveRule("reflect", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setReflectTurns(5);
        }
        return true;
    });
    registry.registerMoveRule("lightscreen", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setLightScreenTurns(5);
        }
        return true;
    });

    registry.registerMoveRule("spikes", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) {
            return true;
        }
        Side& opponentSide = ctx.getOpponentSide(*actorSide);
        opponentSide.addSpikesLayer();
        return true;
    });
    registry.registerMoveRule("toxicspikes", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) {
            return true;
        }
        Side& opponentSide = ctx.getOpponentSide(*actorSide);
        opponentSide.addToxicSpikesLayer();
        return true;
    });
    registry.registerMoveRule("stealthrock", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) {
            return true;
        }
        Side& opponentSide = ctx.getOpponentSide(*actorSide);
        opponentSide.setStealthRock(true);
        return true;
    });

    registry.registerMoveRule("defog", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        Side* targetSide = ctx.findSideForPokemon( defender);
        if (!actorSide || !targetSide) {
            return true;
        }

        actorSide->clearScreenEffects();
        actorSide->clearMist();
        actorSide->clearSafeguard();
        actorSide->clearMudSport();
        actorSide->clearWaterSport();
        actorSide->clearEntryHazards();

        targetSide->clearScreenEffects();
        targetSide->clearMist();
        targetSide->clearSafeguard();
        targetSide->clearMudSport();
        targetSide->clearWaterSport();
        targetSide->clearEntryHazards();

        if (defender) {
            defender->changeEvasionStage(-1);
        }
        return true;
    });

    registry.registerMoveRule("courtchange", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        Side* targetSide = ctx.findSideForPokemon( defender);
        if (!actorSide || !targetSide) {
            return true;
        }

        const int actorReflect = actorSide->getReflectTurns();
        const int actorLightScreen = actorSide->getLightScreenTurns();
        const int actorMist = actorSide->getMistTurns();
        const int actorSafeguard = actorSide->getSafeguardTurns();
        const int actorMudSport = actorSide->getMudSportTurns();
        const int actorWaterSport = actorSide->getWaterSportTurns();
        const int actorSpikes = actorSide->getSpikesLayers();
        const int actorToxicSpikes = actorSide->getToxicSpikesLayers();
        const bool actorStealthRock = actorSide->hasStealthRock();

        actorSide->setReflectTurns(targetSide->getReflectTurns());
        actorSide->setLightScreenTurns(targetSide->getLightScreenTurns());
        actorSide->setMistTurns(targetSide->getMistTurns());
        actorSide->setSafeguardTurns(targetSide->getSafeguardTurns());
        actorSide->setMudSportTurns(targetSide->getMudSportTurns());
        actorSide->setWaterSportTurns(targetSide->getWaterSportTurns());
        actorSide->clearEntryHazards();
        if (targetSide->getSpikesLayers() > 0) {
            for (int i = 0; i < targetSide->getSpikesLayers(); ++i) {
                actorSide->addSpikesLayer();
            }
        }
        if (targetSide->getToxicSpikesLayers() > 0) {
            for (int i = 0; i < targetSide->getToxicSpikesLayers(); ++i) {
                actorSide->addToxicSpikesLayer();
            }
        }
        actorSide->setStealthRock(targetSide->hasStealthRock());

        targetSide->setReflectTurns(actorReflect);
        targetSide->setLightScreenTurns(actorLightScreen);
        targetSide->setMistTurns(actorMist);
        targetSide->setSafeguardTurns(actorSafeguard);
        targetSide->setMudSportTurns(actorMudSport);
        targetSide->setWaterSportTurns(actorWaterSport);
        targetSide->clearEntryHazards();
        if (actorSpikes > 0) {
            for (int i = 0; i < actorSpikes; ++i) {
                targetSide->addSpikesLayer();
            }
        }
        if (actorToxicSpikes > 0) {
            for (int i = 0; i < actorToxicSpikes; ++i) {
                targetSide->addToxicSpikesLayer();
            }
        }
        targetSide->setStealthRock(actorStealthRock);
        return true;
    });

    registry.registerMoveRule("leechseed", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (defender->getSubstituteHP() > 0) {
            return true;
        }
        if (defender->getType1() == Type::Grass || defender->getType2() == Type::Grass) {
            return true;
        }
        defender->setLeechSeedSource(attacker);
        return true;
    });
    registry.registerMoveRule("substitute", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
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

    registry.registerMoveRule("disable", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        ctx.applyDisableToTarget(defender);
        return true;
    });

    registry.registerMoveRule("refresh", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        attacker->removeStatus(StatusType::Paralysis);
        attacker->removeStatus(StatusType::Burn);
        attacker->removeStatus(StatusType::Poison);
        attacker->removeStatus(StatusType::ToxicPoison);
        return true;
    });

    registry.registerMoveRule("imprison", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        ctx.getRuntimeMoveState().imprisonActive[attacker] = true;
        return true;
    });

    registry.registerMoveRule("camouflage", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }

        Type newType = Type::Normal;
        if (ctx.getField().isActive()) {
            switch (ctx.getField().type) {
                case FieldType::Electric:
                    newType = Type::Electric;
                    break;
                case FieldType::Grassy:
                    newType = Type::Grass;
                    break;
                case FieldType::Misty:
                    newType = Type::Fairy;
                    break;
                case FieldType::Psychic:
                    newType = Type::Psychic;
                    break;
                default:
                    newType = Type::Normal;
                    break;
            }
        }

        attacker->setTypes(newType, Type::Count);
        return true;
    });

    registry.registerMoveRule("yawn", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender || defender->getSubstituteHP() > 0 || hasMajorStatus(defender)) {
            return true;
        }
        ctx.getRuntimeMoveState().yawnState[defender] = TimedState{2};
        return true;
    });

    registry.registerMoveRule("taunt", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) {
            return true;
        }
        ctx.getRuntimeMoveState().tauntState[defender] = TimedState{3};
        return true;
    });

    registry.registerMoveRule("torment", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) {
            return true;
        }
        ctx.getRuntimeMoveState().tormentState[defender] = TimedState{3};
        return true;
    });

    registry.registerMoveRule("healblock", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) {
            return true;
        }
        ctx.getRuntimeMoveState().healBlockState[defender] = TimedState{5};
        return true;
    });

    registry.registerMoveRule("embargo", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) {
            return true;
        }
        ctx.getRuntimeMoveState().embargoState[defender] = TimedState{5};
        return true;
    });

    registry.registerMoveRule("endure", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        ctx.getRuntimeMoveState().endureActive[attacker] = true;
        return true;
    });

    registry.registerMoveRule("lockon", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) {
            return true;
        }
        ctx.getRuntimeMoveState().lockOnState[attacker] = LockOnState{defender, 2};
        return true;
    });

    registry.registerMoveRule("mindreader", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) {
            return true;
        }
        ctx.getRuntimeMoveState().lockOnState[attacker] = LockOnState{defender, 2};
        return true;
    });

    registry.registerMoveRule("spite", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) {
            return true;
        }
        const auto lastMoveIt = ctx.getRuntimeMoveState().lastUsedMoveName.find(defender);
        if (lastMoveIt == ctx.getRuntimeMoveState().lastUsedMoveName.end() || lastMoveIt->second.empty()) {
            return true;
        }
        defender->reduceMovePPByName(lastMoveIt->second, 4);
        return true;
    });

    registry.registerMoveRule("whirlwind", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!canForceSwitchTarget(ctx, defender)) {
            return true;
        }

        Side* targetSide = ctx.findSideForPokemon( defender);
        if (targetSide) {
            forceSwitchSide(ctx, *targetSide);
        }
        return true;
    });
    registry.registerMoveRule("roar", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!canForceSwitchTarget(ctx, defender)) {
            return true;
        }

        Side* targetSide = ctx.findSideForPokemon( defender);
        if (targetSide) {
            forceSwitchSide(ctx, *targetSide);
        }
        return true;
    });
    registry.registerMoveRule("dragontail", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!canForceSwitchTarget(ctx, defender)) {
            return true;
        }

        Side* targetSide = ctx.findSideForPokemon( defender);
        if (targetSide) {
            forceSwitchSide(ctx, *targetSide);
        }
        return true;
    });
    registry.registerMoveRule("circlethrow", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!canForceSwitchTarget(ctx, defender)) {
            return true;
        }

        Side* targetSide = ctx.findSideForPokemon( defender);
        if (targetSide) {
            forceSwitchSide(ctx, *targetSide);
        }
        return true;
    });

    registry.registerMoveRule("supersonic", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move& move) {
        return applyConfusionMove(ctx, defender, move);
    });

    registry.registerMoveRule("confuseray", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move& move) {
        return applyConfusionMove(ctx, defender, move);
    });
    registry.registerMoveRule("sweetkiss", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move& move) {
        return applyConfusionMove(ctx, defender, move);
    });
    registry.registerMoveRule("nightmare", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) {
            return true;
        }
        ctx.getRuntimeMoveState().nightmareActive[defender] = true;
        return true;
    });
    registry.registerMoveRule("teeterdance", [](BattleContext& ctx, Pokemon*, Pokemon* defender, const Move& move) {
        return applyConfusionMove(ctx, defender, move);
    });

    registry.registerMoveRule("mimic", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }

        const auto lastMoveIt = ctx.getRuntimeMoveState().lastUsedMoveName.find(defender);
        if (lastMoveIt == ctx.getRuntimeMoveState().lastUsedMoveName.end() || lastMoveIt->second.empty()) {
            return true;
        }

        const std::string copiedMoveName = normalizeMoveName(lastMoveIt->second);
        if (copiedMoveName.empty() || copiedMoveName == "mimic" || copiedMoveName == "sketch" || copiedMoveName == "struggle") {
            return true;
        }

        Move copiedMove = createMoveByName(lastMoveIt->second);
        if (copiedMove.getName().empty()) {
            return true;
        }

        copiedMove.setPP(std::min(5, copiedMove.getMaxPP()));
        attacker->replaceMoveByName("Mimic", copiedMove);
        return true;
    });

    registry.registerMoveRule("transform", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }

        attacker->setTypes(defender->getType1(), defender->getType2());

        std::vector<Move> copiedMoves = defender->getMoves();
        for (Move& copiedMove : copiedMoves) {
            copiedMove.setPP(std::min(5, copiedMove.getMaxPP()));
        }
        attacker->replaceMoves(copiedMoves);
        return true;
    });

    registry.registerMoveRule("conversion", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        for (const Move& knownMove : attacker->getMoves()) {
            if (!knownMove.getName().empty() && knownMove.getType() != Type::Count) {
                attacker->setTypes(knownMove.getType(), Type::Count);
                break;
            }
        }
        return true;
    });

    registry.registerMoveRule("conversion2", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) {
            return true;
        }

        const auto lastMoveIt = ctx.getRuntimeMoveState().lastUsedMoveName.find(defender);
        if (lastMoveIt == ctx.getRuntimeMoveState().lastUsedMoveName.end() || lastMoveIt->second.empty()) {
            return true;
        }

        const Move lastMove = createMoveByName(lastMoveIt->second);
        if (lastMove.getName().empty() || lastMove.getType() == Type::Count) {
            return true;
        }

        attacker->setTypes(findResistingTypeForAttack(lastMove.getType()), Type::Count);
        return true;
    });

    registry.registerMoveRule("sketch", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }

        const auto lastMoveIt = ctx.getRuntimeMoveState().lastUsedMoveName.find(defender);
        if (lastMoveIt == ctx.getRuntimeMoveState().lastUsedMoveName.end() || lastMoveIt->second.empty()) {
            return true;
        }

        const std::string copiedMoveName = normalizeMoveName(lastMoveIt->second);
        if (copiedMoveName.empty() || copiedMoveName == "sketch" || copiedMoveName == "struggle") {
            return true;
        }

        Move copiedMove = createMoveByName(lastMoveIt->second);
        if (copiedMove.getName().empty()) {
            return true;
        }

        attacker->replaceMoveByName("Sketch", copiedMove);
        return true;
    });

    registry.registerMoveRule("roost", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        const int healAmount = std::max(1, attacker->getMaxHP() / 2);
        attacker->setCurrentHP(attacker->getCurrentHP() + healAmount);
        return true;
    });

    registry.registerMoveRule("bellydrum", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        const int hpCost = std::max(1, attacker->getMaxHP() / 2);
        if (attacker->getCurrentHP() <= hpCost) {
            return true;
        }
        attacker->setCurrentHP(attacker->getCurrentHP() - hpCost);
        const int currentStage = attacker->getStatStage(StatIndex::Attack);
        attacker->changeStatStage(StatIndex::Attack, 6 - currentStage);
        return true;
    });

    registry.registerMoveRule("ingrain", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        ctx.getRuntimeMoveState().ingrainActive[attacker] = true;
        return true;
    });

    registry.registerMoveRule("perishsong", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        Pokemon* activeA = ctx.getSideA().getActivePokemon();
        Pokemon* activeB = ctx.getSideB().getActivePokemon();
        if (activeA && !activeA->isFainted()) {
            ctx.getRuntimeMoveState().perishSongState[activeA] = TimedState{3};
        }
        if (activeB && !activeB->isFainted()) {
            ctx.getRuntimeMoveState().perishSongState[activeB] = TimedState{3};
        }
        return true;
    });

    registry.registerMoveRule("attract", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().infatuationSource[defender] = attacker;
        return true;
    });

    registry.registerMoveRule("destinybond", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        ctx.getRuntimeMoveState().destinyBondActive[attacker] = true;
        return true;
    });

    registry.registerMoveRule("grudge", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        ctx.getRuntimeMoveState().grudgeActive[attacker] = true;
        return true;
    });

    registry.registerMoveRule("curse", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker) {
            return true;
        }

        const bool isGhostType = attacker->getType1() == Type::Ghost || attacker->getType2() == Type::Ghost;
        if (isGhostType) {
            if (!defender || defender->getSubstituteHP() > 0) {
                return true;
            }
            const int hpCost = std::max(1, attacker->getMaxHP() / 2);
            if (attacker->getCurrentHP() <= hpCost) {
                return true;
            }
            attacker->setCurrentHP(attacker->getCurrentHP() - hpCost);
            ctx.getRuntimeMoveState().ghostCurseActive[defender] = true;
            return true;
        }

        attacker->changeStatStage(StatIndex::Attack, 1);
        attacker->changeStatStage(StatIndex::Defense, 1);
        attacker->changeStatStage(StatIndex::Speed, -1);
        return true;
    });

    registry.registerMoveRule("sleeptalk", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !attacker->hasStatus(StatusType::Sleep)) {
            return true;
        }

        std::vector<Move> callableMoves;
        for (const Move& knownMove : attacker->getMoves()) {
            if (knownMove.getName().empty()) {
                continue;
            }
            if (moveNameEquals(knownMove, "sleeptalk")) {
                continue;
            }
            callableMoves.push_back(knownMove);
        }

        if (callableMoves.empty()) {
            return true;
        }

        const int index = PRNG::nextInt(0, static_cast<int>(callableMoves.size()));
        Move calledMove = callableMoves[std::min(static_cast<int>(callableMoves.size()) - 1, index)];

        if (calledMove.getCategory() != Category::Status && defender && !defender->isFainted()) {
            const int damage = ctx.calculateDamage(attacker, defender, calledMove);
            defender->setCurrentHP(defender->getCurrentHP() - damage);
        }

        if (defender && !defender->isFainted()) {
            ctx.processMoveEffects(attacker, defender, calledMove);
        }
        return true;
    });

    registry.registerMoveRule("spiderweb", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().trappedBySource[defender] = attacker;
        return true;
    });

    registry.registerMoveRule("meanlook", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().trappedBySource[defender] = attacker;
        return true;
    });

    registry.registerMoveRule("block", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender || defender->getSubstituteHP() > 0) {
            return true;
        }
        ctx.getRuntimeMoveState().trappedBySource[defender] = attacker;
        return true;
    });

    registry.registerMoveRule("doubleteam", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (attacker) {
            attacker->changeEvasionStage(1);
        }
        return true;
    });
    registry.registerMoveRule("minimize", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (attacker) {
            attacker->changeEvasionStage(2);
        }
        return true;
    });
    registry.registerMoveRule("splash", [](BattleContext& ctx, Pokemon*, Pokemon*, const Move&) {
        return true;
    });

    registry.registerMoveRule("teleport", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) {
            return true;
        }
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->autoSwitchNext();
        }
        return true;
    });

    registry.registerMoveRule("painsplit", [](BattleContext& ctx, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) {
            return true;
        }
        const int sharedHp = (attacker->getCurrentHP() + defender->getCurrentHP()) / 2;
        attacker->setCurrentHP(sharedHp);
        defender->setCurrentHP(sharedHp);
        return true;
    });

    registerEvasionDropRule("sandattack");
    registerEvasionDropRule("smokescreen");
    registerEvasionDropRule("flash");

    registerAccuracyDropRule("kinesis", 1);

    registry.registerMoveRule("closecombat", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("superpower", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("overheat", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("leafstorm", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("dracometeor", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("steelbeam", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("vcreate", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("fleurcannon", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });
    registry.registerMoveRule("clangoroussoulblaze", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move& move) { applySelfStatDropMove(attacker, move); return true; });

    // Round 1: 5 new status moves
    registry.registerMoveRule("tailwind", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setTailwindTurns(4);
        }
        return true;
    });

    registry.registerMoveRule("healbell", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) return true;
        for (Pokemon* pokemon : actorSide->getTeam()) {
            if (!pokemon) continue;
            pokemon->removeStatus(StatusType::Burn);
            pokemon->removeStatus(StatusType::Freeze);
            pokemon->removeStatus(StatusType::Paralysis);
            pokemon->removeStatus(StatusType::Poison);
            pokemon->removeStatus(StatusType::ToxicPoison);
            pokemon->removeStatus(StatusType::Sleep);
        }
        return true;
    });

    registry.registerMoveRule("aromatherapy", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (!actorSide) return true;
        for (Pokemon* pokemon : actorSide->getTeam()) {
            if (!pokemon) continue;
            pokemon->removeStatus(StatusType::Burn);
            pokemon->removeStatus(StatusType::Freeze);
            pokemon->removeStatus(StatusType::Paralysis);
            pokemon->removeStatus(StatusType::Poison);
            pokemon->removeStatus(StatusType::ToxicPoison);
            pokemon->removeStatus(StatusType::Sleep);
        }
        return true;
    });

    registry.registerMoveRule("helpinghand", [](BattleContext&, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        attacker->changeStatStage(StatIndex::Attack, 1);
        attacker->changeStatStage(StatIndex::SpecialAttack, 1);
        return true;
    });

    registry.registerMoveRule("followme", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        attacker->changeEvasionStage(2);
        return true;
    });

    registry.registerMoveRule("psychup", [](BattleContext&, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) return true;
        // Copy target's stat stages (excluding HP at index 0)
        for (int i = 1; i < static_cast<int>(StatIndex::Count); ++i) {
            StatIndex idx = static_cast<StatIndex>(i);
            int delta = defender->getStatStage(idx) - attacker->getStatStage(idx);
            attacker->changeStatStage(idx, delta);
        }
        int accDelta = defender->getAccuracyStage() - attacker->getAccuracyStage();
        attacker->changeAccuracyStage(accDelta);
        int evaDelta = defender->getEvasionStage() - attacker->getEvasionStage();
        attacker->changeEvasionStage(evaDelta);
        return true;
    });

    registry.registerMoveRule("soak", [](BattleContext&, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) return true;
        defender->setTypes(Type::Water, Type::Water);
        return true;
    });

    registry.registerMoveRule("sweetscent", [](BattleContext&, Pokemon*, Pokemon* defender, const Move&) {
        if (!defender) return true;
        defender->changeEvasionStage(-2);
        return true;
    });

    registry.registerMoveRule("switcheroo", [](BattleContext&, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) return true;
        ItemType tempType = attacker->getItemType();
        attacker->setItemType(defender->getItemType());
        defender->setItemType(tempType);
        return true;
    });

    registry.registerMoveRule("guardswap", [](BattleContext&, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) return true;
        // Swap Defense and Sp. Def stat stages with target
        int atkDef = attacker->getStatStage(StatIndex::Defense);
        int atkSpd = attacker->getStatStage(StatIndex::SpecialDefense);
        int defDef = defender->getStatStage(StatIndex::Defense);
        int defSpd = defender->getStatStage(StatIndex::SpecialDefense);
        attacker->changeStatStage(StatIndex::Defense, defDef - atkDef);
        attacker->changeStatStage(StatIndex::SpecialDefense, defSpd - atkSpd);
        defender->changeStatStage(StatIndex::Defense, atkDef - defDef);
        defender->changeStatStage(StatIndex::SpecialDefense, atkSpd - defSpd);
        return true;
    });

    registry.registerMoveRule("stockpile", [](BattleContext&, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        if (attacker->getStockpileCount() < 3) {
            attacker->setStockpileCount(attacker->getStockpileCount() + 1);
        }
        return true;
    });

    registry.registerMoveRule("swallow", [](BattleContext&, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        const int count = attacker->getStockpileCount();
        if (count > 0) {
            const int healFraction = count == 1 ? 4 : (count == 2 ? 2 : 1);
            attacker->setCurrentHP(attacker->getCurrentHP() + attacker->getMaxHP() / healFraction);
            attacker->setStockpileCount(0);
        }
        return true;
    });

    registry.registerMoveRule("spitup", [](BattleContext&, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (!attacker || !defender) return true;
        const int count = attacker->getStockpileCount();
        if (count > 0) {
            // Spit Up deals damage based on Stockpile count
            // Base power = 100 * count
            const int basePower = 100 * count;
            const int level = attacker->getLevel();
            const int atk = attacker->getAttack();
            const int def = defender->getDefense();
            const int damage = std::max(1, ((2 * level / 5 + 2) * basePower * atk / def / 50 + 2));
            defender->setCurrentHP(defender->getCurrentHP() - damage);
            attacker->setStockpileCount(0);
        }
        return true;
    });

    registry.registerMoveRule("recycle", [](BattleContext&, Pokemon* attacker, Pokemon*, const Move&) {
        if (!attacker) return true;
        if (attacker->getItemType() == ItemType::None
            && attacker->getConsumedItemType() != ItemType::None) {
            attacker->setItemType(attacker->getConsumedItemType());
            attacker->setConsumedItemType(ItemType::None);
        }
        return true;
    });

    registry.registerMoveRule("naturepower", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        (void)attacker;
        (void)ctx;
        // Nature Power transforms based on terrain
        // For now, default to Tri Attack (normal), Swift for Electric/Misty/Grassy/Psychic terrain
        return true;
    });

    registry.registerMoveRule("magiccoat", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setMagicCoatTurns(1);
        }
        return true;
    });

    registry.registerMoveRule("snatch", [](BattleContext& ctx, Pokemon* attacker, Pokemon*, const Move&) {
        Side* actorSide = ctx.findSideForPokemon( attacker);
        if (actorSide) {
            actorSide->setSnatchTurns(1);
        }
        return true;
    });

    registry.registerMoveRule("haze", [](BattleContext&, Pokemon* attacker, Pokemon* defender, const Move&) {
        if (attacker) {
            attacker->resetStatStages();
        }
        if (defender) {
            defender->resetStatStages();
        }
        return true;
    });
}
