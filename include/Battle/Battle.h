#pragma once

#include "Side.h"
#include "Field.h"
#include "Weather.h"
#include "BattleQueue.h"
#include "EventSystem.h"
#include "BattleActions.h"
#include "Abilities.h" 
#include "Items.h" 
#include "Moves.h"
#include <memory>

class Battle {
public:
    Battle(Side sideA, Side sideB);

    Side& getSideA();
    Side& getSideB();
    Side& getOpponentSide(const Side& side);
    Field& getField();
    Weather& getWeather();
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
private:
    Side sideA;
    Side sideB;
    Field field;
    Weather weather;
    BattleQueue queue;
    EventSystem eventSystem;
    int turnNumber = 0;
};
