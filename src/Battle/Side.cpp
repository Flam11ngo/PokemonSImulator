#include "Battle/Side.h"

Side::Side(std::string name) : name(std::move(name)), count(0), activeIndex(0) {}

bool Side::addPokemon(Pokemon* pokemon) {
    if (count >= 6 || pokemon == nullptr) {
        return false;
    }
    team[count++] = pokemon;
    return true;
}

Pokemon* Side::getActivePokemon() const {
    if (count == 0 || activeIndex < 0 || activeIndex >= count) {
        return nullptr;
    }
    return team[activeIndex];
}

bool Side::canSwitch() const {
    for (int i = 0; i < count; ++i) {
        if (i != activeIndex && team[i] != nullptr && !team[i]->isFainted()) {
            return true;
        }
    }
    return false;
}

bool Side::switchActive(int newIndex) {
    if (newIndex < 0 || newIndex >= count || newIndex == activeIndex) {
        return false;
    }
    if (team[newIndex] == nullptr || team[newIndex]->isFainted()) {
        return false;
    }
    activeIndex = newIndex;
    return true;
}

bool Side::autoSwitchNext() {
    for (int i = 0; i < count; ++i) {
        if (i != activeIndex && team[i] != nullptr && !team[i]->isFainted()) {
            return switchActive(i);
        }
    }
    return false;
}

bool Side::hasRemainingPokemon() const {
    for (int i = 0; i < count; ++i) {
        if (team[i] != nullptr && !team[i]->isFainted()) {
            return true;
        }
    }
    return false;
}

bool Side::isEmpty() const {
    return count == 0;
}

void Side::addFieldEffect(const FieldEffect& effect) {
    fieldEffects.push_back(effect);
}

void Side::clearFieldEffects() {
    fieldEffects.clear();
}
