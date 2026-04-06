#pragma once

#include "Types.h"
#include "Status.h"
#include <string>

// 技能效果类型枚举
enum class MoveEffect {
    None,           // 无特殊效果
    Pursuit,        // 追击效果
    Status,         // 状态变化
    StatChange,     // 能力变化
    Recoil,         // 反伤
    Drain,          // 吸血
    Flinch,         // 畏缩
    Paralyze,       // 麻痹
    Sleep,          // 睡眠
    Freeze,         // 冰冻
    Burn,           // 烧伤
    Poison,         // 中毒
    Confuse,        // 混乱
    LeechSeed,      // 寄生种子
    Reflect,        // 反射壁
    LightScreen,    // 光墙
    Safeguard,      // 守护
    Tailwind,       // 顺风
    StealthRock,    // 隐形岩
    Spikes,         // 撒菱
    ToxicSpikes     // 毒菱
};

// 能力变化类型枚举
enum class StatChangeType {
    Attack,         // 攻击
    Defense,        // 防御
    SpAttack,       // 特攻
    SpDefense,      // 特防
    Speed,          // 速度
    Accuracy,       // 命中
    Evasion         // 闪避
};

class Move {
private:
    std::string name;
    Type type;
    Category category;
    int power;
    int accuracy; // 0-100
    int pp; // Power Points
    int maxPP;
    MoveEffect effect; // 技能效果
    int effectChance; // 效果触发概率（0-100）
    int effectParam1; // 效果参数1
    int effectParam2; // 效果参数2

public:
    Move();
    Move(std::string name, Type type, Category category, int power, int accuracy, int pp);
    Move(std::string name, Type type, Category category, int power, int accuracy, int pp, MoveEffect effect, int effectChance, int effectParam1 = 0, int effectParam2 = 0);
    Move(const Move& other);
    Move& operator=(const Move& other);
    ~Move();
    // Getters
    const std::string& getName() const { return name; }
    const Type& getType() const { return type; }
    const Category& getCategory() const { return category; }
    const int& getPower() const { return power; }
    const int& getAccuracy() const { return accuracy; }
    const int& getPP() const { return pp; }
    const int& getMaxPP() const { return maxPP; }
    const MoveEffect& getEffect() const { return effect; }
    const int& getEffectChance() const { return effectChance; }
    const int& getEffectParam1() const { return effectParam1; }
    const int& getEffectParam2() const { return effectParam2; }

    // Setters
    void setPP(int p) { pp = p; if (pp < 0) pp = 0; if (pp > maxPP) pp = maxPP; }
    void setEffect(MoveEffect e) { effect = e; }
    void setEffectChance(int chance) { effectChance = chance; if (effectChance < 0) effectChance = 0; if (effectChance > 100) effectChance = 100; }
    void setEffectParam1(int param) { effectParam1 = param; }
    void setEffectParam2(int param) { effectParam2 = param; }

    // Utility
    bool canUse() const { return pp > 0; }
    void use() { if (pp > 0) pp--; }
    bool isStatusMove() const { return category == Category::Status; }
    bool isPursuit() const { return effect == MoveEffect::Pursuit; }
};
