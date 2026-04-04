#include "Battle/BattleQueue.h"
#include <algorithm>

void BattleQueue::push(const BattleAction& action, bool trickRoomEnabled) {
    trickRoom = trickRoomEnabled;
    actions.push_back(action);
}

BattleAction BattleQueue::pop() {
    if (actions.empty()) {
        return BattleAction::makePass(nullptr);
    }
    std::sort(actions.begin(), actions.end(), [this](const BattleAction& a, const BattleAction& b) {
        if (a.priority != b.priority) {
            if (trickRoom) {
                return a.priority < b.priority;
            }
            return a.priority > b.priority;
        }
        if (a.actor && b.actor) {
            return a.actor->getName() < b.actor->getName();
        }
        return a.actor != nullptr;
    });
    BattleAction front = actions.front();
    actions.erase(actions.begin());
    return front;
}

bool BattleQueue::empty() const {
    return actions.empty();
}

void BattleQueue::clear() {
    actions.clear();
}

void BattleQueue::setTrickRoom(bool enable) {
    trickRoom = enable;
}
