#pragma once

#include "BattleData.h"  // IWYU pragma: keep
#include "Side.h"
#include "Field.h"
#include "Weather.h"
#include "BattleQueue.h"
#include "EventSystem.h"
#include "BattleActions.h"
#include "MoveRuleRegistry.h"
#include <string>
#include <unordered_map>

class Battle {
public:
    Battle(Side sideA, Side sideB);

    Side& getSideA();
    const Side& getSideA() const;
    Side& getSideB();
    const Side& getSideB() const;
    Side& getOpponentSide(const Side& side);
    Field& getField();
    Weather& getWeather();
    EventSystem& getEventSystem();
    const EventSystem& getEventSystem() const;

    void enqueueAction(const BattleAction& action);
    void processTurn();
    void resolveNextAction();
    void applyStatusEffects();
    void applyWeatherEffects();
    void applyFieldEffects();

    int calculateDamage(Pokemon* attacker, Pokemon* defender, const Move& move) const;
    bool switchPokemon(Side& side, int newIndex);
    
    // 特性相关方法
    void triggerAbility(Pokemon* pokemon, Trigger trigger, Pokemon* opponent = nullptr, void* context = nullptr);
    void triggerAbilities(Trigger trigger, Pokemon* target = nullptr);
    
    // 物品相关方法
    void triggerItemEffect(Pokemon* pokemon, ItemTrigger trigger, Pokemon* opponent, void* context = nullptr);
    void triggerItemEffects(ItemTrigger trigger, Pokemon* target = nullptr);
    
    // 处理技能追加效果
    void processMoveEffects(Pokemon* attacker, Pokemon* defender, const Move& move);
    
    // 数值修改方法
    float applyStatModifiers(Pokemon* pokemon, float baseValue, const std::string& statName) const;
    float applyDamageModifiers(Pokemon* attacker, Pokemon* defender, const Move& move, float baseModifier) const;
    
    // 免疫检查方法
    bool isImmuneToStatus(Pokemon* pokemon, StatusType status) const;
    bool isImmuneToType(Pokemon* pokemon, int typeId, bool& shouldHeal, int& healPercent) const;
    bool isImmuneToWeather(Pokemon* pokemon, const Weather& weather) const;
    bool checkTypeImmunity(Pokemon* defender, int typeId, Pokemon* attacker);
    
    // 获取当前回合数
    int getTurnNumber() const { return turnNumber; }
    
    // 获取对手的宝可梦（辅助方法）
    Pokemon* getOpponentPokemon(Pokemon* self) const;
    static Side* findSideForPokemon(Battle& battle, Pokemon* pokemon);
    static const Side* findSideForPokemon(const Battle& battle, Pokemon* pokemon);
public:
    enum class SemiInvulnerableState {
        None,
        Underground,
        Airborne,
        Underwater,
        Phased,
    };

private:
    struct EncoreState {
        std::string lockedMoveName;
        int remainingTurns = 0;
    };

    struct RuntimeMoveState {
        std::unordered_map<Pokemon*, std::string> lastUsedMoveName;
        std::unordered_map<Pokemon*, EncoreState> encoreState;
        std::unordered_map<Pokemon*, std::string> chargingMoveName;
        std::unordered_map<Pokemon*, SemiInvulnerableState> semiInvulnerableState;
        std::unordered_map<Pokemon*, bool> typeShiftUsed;
        Pokemon* pursuitSwitchTarget = nullptr;
        bool roundUsedThisTurn = false;
    };

    void clearPokemonRuntimeState(Pokemon* pokemon);
    void clearSideRuntimeState(const Side& side);
    void tickEncoreForActor(Pokemon* actor);
    SemiInvulnerableState getSemiInvulnerableState(const Pokemon* pokemon) const;
    void beginTurn();
    void endTurn();
    void resetActiveProtection();
    Move applyEncoreOverride(Pokemon* actor, const Move& intendedMove) const;
    bool handleTwoTurnChargeTurn(Pokemon* actor, const Move& selectedMove);
    void recordExecutedMove(Pokemon* actor, const Move& selectedMove);
    void handlePursuitOnSwitch(Pokemon* switchingPokemon, Side* switchingSide);
    void applyEntryHazardsOnSwitchIn(Side* enteringSide, Pokemon* enteringPokemon);
    void initializeCoreMoveRules();

    Side sideA;
    Side sideB;
    Field field;
    Weather weather;
    BattleQueue queue;
    EventSystem eventSystem;
    MoveRuleRegistry moveRuleRegistry;
    RuntimeMoveState runtimeMoveState;
    int turnNumber = 0;
};
