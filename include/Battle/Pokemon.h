#pragma once

#include "Types.h"
#include "Natures.h"
#include "Abilities.h"
#include "Items.h"
#include "Status.h"
#include "Moves.h"
#include "Species.h"
#include <string>
#include <vector>
#include <array>

enum class StatIndex {
    HP,
    Attack,
    Defense,
    SpecialAttack,
    SpecialDefense,
    Speed,
    Count
};

class Pokemon {
private:
    const Species* species;
    std::string name;
    Type type1;
    Type type2; // Use Type::Count for no second type
    Nature nature;
    AbilityType ability;
    ItemType itemType;
    int level;
    int maxHP;
    int currentHP;
    int attack;
    int defense;
    int specialAttack;
    int specialDefense;
    int speed;
    std::vector<std::pair<StatusType, int>> statuses;
    std::vector<Move> moves;
    std::array<int, static_cast<int>(StatIndex::Count)> ivs;
    std::array<int, static_cast<int>(StatIndex::Count)> evs;

    int calculateStat(int base, int iv, int ev, int level, float natureModifier, bool isHP) const;
    void recalculateStats();

public:
    Pokemon(const Species* species, Nature nat, AbilityType abil,
            int lvl,
            const std::array<int, static_cast<int>(StatIndex::Count)>& ivs,
            const std::array<int, static_cast<int>(StatIndex::Count)>& evs);

    // Getters
    const Species* getSpecies() const { return species; }
    std::string getName() const { return name; }
    Type getType1() const { return type1; }
    Type getType2() const { return type2; }
    Nature getNature() const { return nature; }
    AbilityType getAbility() const { return ability; }
    int getLevel() const { return level; }
    int getMaxHP() const { return maxHP; }
    int getCurrentHP() const { return currentHP; }
    int getAttack() const { return attack; }
    int getDefense() const { return defense; }
    int getSpecialAttack() const { return specialAttack; }
    int getSpecialDefense() const { return specialDefense; }
    int getSpeed() const { return speed; }
    const std::vector<std::pair<StatusType, int>>& getStatuses() const { return statuses; }
    void addStatus(StatusType s, int duration = -1);
    void removeStatus(StatusType s);
    void clearStatuses();
    bool hasStatus(StatusType s) const;
    const std::vector<Move>& getMoves() const { return moves; }
    int getIV(StatIndex index) const { return ivs[static_cast<int>(index)]; }
    int getEV(StatIndex index) const { return evs[static_cast<int>(index)]; }
    ItemType getItemType() const { return itemType; }
    Item getHeldItem() const { return getItem(itemType); }

    // Setters
    void setCurrentHP(int hp) { currentHP = hp; if (currentHP < 0) currentHP = 0; if (currentHP > maxHP) currentHP = maxHP; }
    void setStatus();
    void setItemType(ItemType item) { itemType = item; }
    void removeItem() { itemType = ItemType::None; }
    void setIV(StatIndex index, int value) { ivs[static_cast<int>(index)] = value; recalculateStats(); }
    void setEV(StatIndex index, int value) { evs[static_cast<int>(index)] = value; recalculateStats(); }
    void addMove(const Move& move) { if (moves.size() < 4) moves.push_back(move); }    
    // Utility
    bool isFainted() const { return currentHP <= 0; }
    float getTypeEffectiveness(Type attackType) const;
    float getNatureModifier(int statIndex) const { return ::getNatureModifier(nature, statIndex); }
};