#include "Battle/EventSystem.h"

void EventSystem::recordEvent(const BattleEvent& event) {
    events.push_back(event);
}

const std::vector<BattleEvent>& EventSystem::getEvents() const {
    return events;
}

void EventSystem::clear() {
    events.clear();
}
