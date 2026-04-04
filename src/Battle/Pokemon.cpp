#include "Battle/Pokemon.h"
#include "Battle/Utils.h"

Pokemon::Pokemon(const Species* species, Nature nat, AbilityType abil,
                 int lvl,
                 const std::array<int, static_cast<int>(StatIndex::Count)>& ivs,
                 const std::array<int, static_cast<int>(StatIndex::Count)>& evs)
    : species(species), name(species ? species->name : "Unknown"),
      type1(species ? species->type1 : Type::Normal),
      type2(species ? species->type2 : Type::Count),
      nature(nat), ability(abil), itemType(ItemType::None), level(lvl), statuses(),
      ivs(ivs), evs(evs) {
    recalculateStats();
    currentHP = maxHP;
}

int Pokemon::calculateStat(int base, int iv, int ev, int level, float natureModifier, bool isHP) const {
    if (isHP) {
        if (base <= 1) {
            return 1;
        }
        int value = ((2 * base + iv + ev / 4) * level) / 100 + level + 10;
        return value;
    }
    int value = ((2 * base + iv + ev / 4) * level) / 100 + 5;
    return static_cast<int>(value * natureModifier);
}

void Pokemon::recalculateStats() {
    if (!species) {
        maxHP = currentHP = 1;
        attack = defense = specialAttack = specialDefense = speed = 1;
        return;
    }

    maxHP = calculateStat(species->baseStats[0], ivs[static_cast<int>(StatIndex::HP)], evs[static_cast<int>(StatIndex::HP)], level, 1.0f, true);
    attack = calculateStat(species->baseStats[1], ivs[static_cast<int>(StatIndex::Attack)], evs[static_cast<int>(StatIndex::Attack)], level, getNatureModifier(static_cast<int>(StatIndex::Attack) - 1), false);
    defense = calculateStat(species->baseStats[2], ivs[static_cast<int>(StatIndex::Defense)], evs[static_cast<int>(StatIndex::Defense)], level, getNatureModifier(static_cast<int>(StatIndex::Defense) - 1), false);
    specialAttack = calculateStat(species->baseStats[3], ivs[static_cast<int>(StatIndex::SpecialAttack)], evs[static_cast<int>(StatIndex::SpecialAttack)], level, getNatureModifier(static_cast<int>(StatIndex::SpecialAttack) - 1), false);
    specialDefense = calculateStat(species->baseStats[4], ivs[static_cast<int>(StatIndex::SpecialDefense)], evs[static_cast<int>(StatIndex::SpecialDefense)], level, getNatureModifier(static_cast<int>(StatIndex::SpecialDefense) - 1), false);
    speed = calculateStat(species->baseStats[5], ivs[static_cast<int>(StatIndex::Speed)], evs[static_cast<int>(StatIndex::Speed)], level, getNatureModifier(static_cast<int>(StatIndex::Speed) - 1), false);
}

float Pokemon::getTypeEffectiveness(Type attackType) const {
    float effectiveness = Utils::TypeEffectiveness[static_cast<int>(attackType)][static_cast<int>(type1)];
    if (type2 != Type::Count) {
        effectiveness *= Utils::TypeEffectiveness[static_cast<int>(attackType)][static_cast<int>(type2)];
    }
    return effectiveness;
}

void Pokemon::addStatus(StatusType s, int duration) {
    if (duration == -1) {
        if (s == StatusType::Sleep) {
            duration = 1; // Default sleep duration
        } else {
            duration = -1; // Permanent
        }
    }
    statuses.emplace_back(s, duration);
}

void Pokemon::removeStatus(StatusType s) {
    statuses.erase(std::remove_if(statuses.begin(), statuses.end(),
                                  [s](const std::pair<StatusType, int>& status) {
                                      return status.first == s;
                                  }), statuses.end());
}

void Pokemon::clearStatuses() {
    statuses.clear();
}

bool Pokemon::hasStatus(StatusType s) const {
    return std::any_of(statuses.begin(), statuses.end(),
                       [s](const std::pair<StatusType, int>& status) {
                           return status.first == s;
                       });
}
