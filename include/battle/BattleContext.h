#pragma once

#include "battle/Side.h"
#include "battle/Field.h"
#include "battle/Weather.h"
#include "battle/EventSystem.h"
#include "battle/OnCondition.h"
#include "battle/Items.h"

#include <nlohmann/json.hpp>
#include <vector>

class Pokemon;
class Battle;
struct RuntimeMoveState;

class BattleContext {
public:
    BattleContext(Weather& weather, Field& field,
                  Side& sideA, Side& sideB,
                  EventSystem& eventSystem,
                  RuntimeMoveState& runtimeState,
                  int& turnNumber,
                  std::vector<nlohmann::json>& specialEvents);

    // --- Subsystem access ---
    Weather& getWeather() { return weather_; }
    const Weather& getWeather() const { return weather_; }
    Field& getField() { return field_; }
    const Field& getField() const { return field_; }
    Side& getSideA() { return sideA_; }
    const Side& getSideA() const { return sideA_; }
    Side& getSideB() { return sideB_; }
    const Side& getSideB() const { return sideB_; }
    Side& getOpponentSide(const Side& side);

    // --- Turn info ---
    int getTurnNumber() const { return turnNumber_; }

    // --- Side/Pokemon lookup ---
    Side* findSideForPokemon(Pokemon* pokemon);
    const Side* findSideForPokemon(const Pokemon* pokemon) const;
    Pokemon* getOpponentPokemon(Pokemon* self) const;

    // --- Runtime state (needed by move rules) ---
    RuntimeMoveState& getRuntimeMoveState() { return runtimeState_; }
    const RuntimeMoveState& getRuntimeMoveState() const { return runtimeState_; }

    // --- Event recording ---
    EventSystem& getEventSystem() { return eventSystem_; }
    const EventSystem& getEventSystem() const { return eventSystem_; }
    void appendSpecialEvent(const std::string& eventType, const nlohmann::json& details);

    // --- Trigger chaining (delegates to Battle via back-pointer) ---
    void setBattlePointer(Battle* battle);
    void triggerAbility(Pokemon* pokemon, Trigger trigger,
                        Pokemon* opponent = nullptr, void* context = nullptr);
    void triggerItemEffect(Pokemon* pokemon, ItemTrigger trigger,
                           Pokemon* opponent, void* context = nullptr);

    // --- Battle operations needed by move rules (delegated via back-pointer) ---
    bool switchPokemon(Side& side, int newIndex);
    bool canBeForcedToSwitch(Pokemon* defender) const;
    int calculateDamage(Pokemon* attacker, Pokemon* defender, const Move& move) const;
    void processMoveEffects(Pokemon* attacker, Pokemon* defender, const Move& move);
    void applyDisableToTarget(Pokemon* target);

private:
    Weather& weather_;
    Field& field_;
    Side& sideA_;
    Side& sideB_;
    EventSystem& eventSystem_;
    RuntimeMoveState& runtimeState_;
    int& turnNumber_;
    std::vector<nlohmann::json>& specialEvents_;
    Battle* battle_ = nullptr;
};
