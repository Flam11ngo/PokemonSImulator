#pragma once

#include "OnCondition.h"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

class Pokemon;
class Battle;
class Move;

// 物品触发时机（扩展以匹配特性系统）
enum class ItemTrigger {
    OnEntry,        // 出场时
    OnTurnStart,    // 回合开始时
    OnTurnEnd,      // 回合结束时
    OnDamage,       // 受到伤害时
    OnDealDamage,   // 造成伤害时
    AfterDamage,    // 受到伤害后
    OnAttack,       // 攻击时
    OnFaint,        // 濒死时
    OnSwitchOut,    // 退场时
    OnStatChange,   // 能力变化时
    OnStatus,       // 获得状态时
    OnEat,          // 食用时（树果）
    Count
};

enum class ItemType {
    None,
    // 树果
    OranBerry,
    SitrusBerry,
    LumBerry,
    ChestoBerry,
    PechaBerry,
    RawstBerry,
    AspearBerry,
    PersimBerry,
    
    // 回复类
    Leftovers,
    BlackSludge,
    ShellBell,
    
    // 提升类
    ChoiceBand,
    ChoiceSpecs,
    ChoiceScarf,
    LifeOrb,
    ExpertBelt,
    MuscleBand,
    WiseGlasses,
    
    // 防御类
    FocusSash,
    RockyHelmet,
    AirBalloon,
    Eviolite,
    AssaultVest,
    
    // 战术类
    RedCard,
    EjectButton,
    WeaknessPolicy,
    BerryJuice,
    
    Count
};

// 数值修改器
struct ItemStatModifier {
    enum Stat { Attack, Defense, SpAttack, SpDefense, Speed, Accuracy, Evasion };
    Stat stat;
    float multiplier;  // 倍率
    int flatBonus;     // 固定加成
    
    ItemStatModifier() : multiplier(1.0f), flatBonus(0) {}
    ItemStatModifier(Stat s, float mult = 1.0f, int bonus = 0) 
        : stat(s), multiplier(mult), flatBonus(bonus) {}
};

// 伤害修改器
struct ItemDamageModifier {
    float multiplier;
    bool onDealDamage;   // true: 造成伤害时, false: 受到伤害时
    std::function<bool(Pokemon* self, Pokemon* other, const Move& move)> condition;
    
    ItemDamageModifier() : multiplier(1.0f), onDealDamage(true), condition(nullptr) {}
    ItemDamageModifier(float mult, bool onDeal, 
                      std::function<bool(Pokemon*, Pokemon*, const Move&)> cond = nullptr) 
        : multiplier(mult), onDealDamage(onDeal), condition(cond) {}
};

// 回复效果
struct HealEffect {
    enum HealType { Fixed, Percentage, Fraction };
    HealType type;
    int value;          // 固定值或百分比
    int numerator;      // 分子（用于分数，如1/16）
    int denominator;    // 分母（用于分数）
    
    HealEffect() : type(Fixed), value(0), numerator(0), denominator(0) {}
    HealEffect(int fixedValue) : type(Fixed), value(fixedValue), numerator(0), denominator(0) {}
    HealEffect(int num, int den) : type(Fraction), value(0), numerator(num), denominator(den) {}
    static HealEffect percent(int pct) { 
        HealEffect h; 
        h.type = Percentage; 
        h.value = pct; 
        return h; 
    }
    
    int calculate(int maxHP, int currentHP) const {
        switch (type) {
            case Fixed:
                return value;
            case Percentage:
                return maxHP * value / 100;
            case Fraction:
                return std::max(1, maxHP * numerator / denominator);
        }
        return 0;
    }
};

// 能力变化效果
struct StatChangeEffect {
    ItemStatModifier::Stat stat;
    int stages;  // 变化等级 (-6 到 6)
    
    StatChangeEffect() : stat(ItemStatModifier::Stat::Attack), stages(0) {}
    StatChangeEffect(ItemStatModifier::Stat s, int stg) : stat(s), stages(stg) {}
};

class Item {
public:
    ItemType type;
    std::string name;
    
    // 1. 基础效果（多个触发时机）
    std::unordered_map<ItemTrigger, std::function<void(Pokemon* self, Pokemon* opponent, Battle& battle, void* context)>> effects;
    
    // 2. 数值修改
    std::vector<ItemStatModifier> statModifiers;
    
    // 3. 伤害修改
    ItemDamageModifier damageModifier;
    
    // 4. 回复效果
    HealEffect healEffect;
    
    // 5. 能力变化
    std::vector<StatChangeEffect> statChanges;
    
    // 6. 一次性使用标记
    bool isConsumable;
    bool isUsed;  // 是否已使用（对于一次性物品）
    
    // 构造函数
    Item() : type(ItemType::None), name("无"), isConsumable(false), isUsed(false) {}
    Item(ItemType t, const std::string& n) : type(t), name(n), isConsumable(false), isUsed(false) {}
    
    // 辅助方法
    bool hasTrigger(ItemTrigger trigger) const {
        return effects.find(trigger) != effects.end();
    }
    
    void executeTrigger(ItemTrigger trigger, Pokemon* self, Pokemon* opponent, Battle& battle, void* context = nullptr);
    
    // 添加效果的方法
    void addEffect(ItemTrigger trigger, std::function<void(Pokemon*, Pokemon*, Battle&, void*)> effect) {
        effects[trigger] = effect;
    }
    
    void addStatModifier(ItemStatModifier::Stat stat, float multiplier, int flatBonus = 0) {
        statModifiers.push_back(ItemStatModifier(stat, multiplier, flatBonus));
    }
    
    void setDamageModifier(float multiplier, bool onDealDamage,
                          std::function<bool(Pokemon*, Pokemon*, const Move&)> condition = nullptr) {
        damageModifier = ItemDamageModifier(multiplier, onDealDamage, condition);
    }
    
    void setHealEffect(int numerator, int denominator) {
        healEffect = HealEffect(numerator, denominator);
    }
    
    void setHealEffectFixed(int value) {
        healEffect = HealEffect(value);
    }
    
    void addStatChange(ItemStatModifier::Stat stat, int stages) {
        statChanges.push_back(StatChangeEffect(stat, stages));
    }
    
    // 应用数值修改
    float applyStatModifier(float baseValue, ItemStatModifier::Stat stat) const {
        float modified = baseValue;
        for (const auto& mod : statModifiers) {
            if (mod.stat == stat) {
                modified = modified * mod.multiplier + mod.flatBonus;
            }
        }
        return modified;
    }
    
    // 应用伤害修改
    float applyDamageModifier(float damage, Pokemon* self, Pokemon* other, const Move& move, bool isDealing) const {
        if (damageModifier.multiplier == 1.0f) return damage;
        if (damageModifier.onDealDamage != isDealing) return damage;
        if (damageModifier.condition && !damageModifier.condition(self, other, move)) return damage;
        return damage * damageModifier.multiplier;
    }
    
    // 检查是否可以使用
    bool canUse(Pokemon* self) const {
        if (isUsed && isConsumable) return false;
        // 可以添加其他条件，如HP要求等
        return true;
    }
};

// 物品工厂函数
Item getItem(ItemType type);
Item createOranBerry();
Item createSitrusBerry();
Item createLumBerry();
Item createLeftovers();
Item createChoiceBand();
Item createChoiceSpecs();
Item createChoiceScarf();
Item createLifeOrb();
Item createFocusSash();
Item createRockyHelmet();
Item createAirBalloon();
Item createEviolite();
Item createAssaultVest();
Item createRedCard();
Item createWeaknessPolicy();

// 辅助函数
std::string getItemName(ItemType type);
bool isBerry(ItemType type);  // 判断是否为树果