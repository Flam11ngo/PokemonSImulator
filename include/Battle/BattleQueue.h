#pragma once

#include "BattleActions.h"
#include <vector>

class BattleQueue {
public:
    void push(const BattleAction& action, bool trickRoom = false);
    BattleAction pop();
    bool empty() const;
    void clear();
    void setTrickRoom(bool enable);

private:
    std::vector<BattleAction> actions;
    bool trickRoom = false;
};
