#pragma once

#include "Pokemon.h"
#include "Effect.h"
#include <array>
#include <string>
#include <vector>

class Side {
public:
    Side() = default;
    explicit Side(std::string name);

    bool addPokemon(Pokemon* pokemon);
    Pokemon* getActivePokemon() const;
    int getActiveIndex() const { return activeIndex; }
    bool canSwitch() const;
    bool switchActive(int newIndex);
    bool autoSwitchNext();
    bool hasRemainingPokemon() const;
    bool isEmpty() const;

    const std::string& getName() const { return name; }
    const std::vector<FieldEffect>& getFieldEffects() const { return fieldEffects; }

    void addFieldEffect(const FieldEffect& effect);
    void clearFieldEffects();

    const std::array<Pokemon*, 6>& getTeam() const { return team; }
    int getPokemonCount() const { return count; }
    
    // 连续守护相关方法
    int getProtectCount() const { return protectCount; }
    void setProtectCount(int count) { protectCount = count; }
    void resetProtectCount() { protectCount = 0; }

private:
    std::string name;
    std::array<Pokemon*, 6> team{};
    int count = 0;
    int activeIndex = 0;
    std::vector<FieldEffect> fieldEffects;
    int protectCount = 0; // 连续守护的次数
};
