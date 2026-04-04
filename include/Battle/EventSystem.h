#pragma once

#include "BattleActions.h"
#include <string>
#include <vector>

struct ActionLog {
    int turnNumber = 0;
    std::string actorName;
    std::string targetName;
    std::string actionName;
    std::string itemName;
};

struct BattleEvent {
    int turnNumber = 0;
    ActionLog sideAAction;
    ActionLog sideBAction;
};

class EventSystem {
public:
    void recordEvent(const BattleEvent& event);
    const std::vector<BattleEvent>& getEvents() const;
    void clear();

private:
    std::vector<BattleEvent> events;
};
