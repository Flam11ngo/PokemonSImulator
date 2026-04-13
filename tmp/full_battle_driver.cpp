#include "Battle/BattleSession.h"
#include "Battle/BattleToJson.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

namespace {
std::string normalize(const std::string& value) {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == ' ' || ch == '-' || ch == '_' || ch == '\'') {
            continue;
        }
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return out;
}

bool isBattleOver(const Battle& battle) {
    auto countAlive = [](const Side& side) {
        int alive = 0;
        for (Pokemon* pokemon : side.getTeam()) {
            if (pokemon && !pokemon->isFainted()) {
                ++alive;
            }
        }
        return alive;
    };

    return countAlive(battle.getSideA()) == 0 || countAlive(battle.getSideB()) == 0;
}

std::optional<Move> chooseLongBattleMove(const Pokemon* attacker, const Pokemon* defender) {
    if (!attacker) {
        return std::nullopt;
    }

    const auto& moves = attacker->getMoves();
    if (moves.empty()) {
        return std::nullopt;
    }

    std::optional<Move> bestMove;
    float bestScore = 1e9f;

    for (const Move& move : moves) {
        const int power = move.getPower();
        if (power <= 0) {
            continue;
        }

        float score = static_cast<float>(power);
        if (defender) {
            score *= defender->getTypeEffectiveness(move.getType());
        }

        if (!bestMove.has_value() || score < bestScore) {
            bestMove = move;
            bestScore = score;
        }
    }

    if (bestMove.has_value()) {
        return bestMove;
    }
    return moves.front();
}

std::optional<int> chooseSwitchIndex(const Side& side) {
    const int activeIndex = side.getActiveIndex();
    const auto& team = side.getTeam();
    for (int i = 0; i < static_cast<int>(team.size()); ++i) {
        if (i == activeIndex) {
            continue;
        }
        Pokemon* candidate = team[i];
        if (candidate && !candidate->isFainted()) {
            return i;
        }
    }
    return std::nullopt;
}

nlohmann::json buildTurnRequest(Battle& battle) {
    nlohmann::json turn;
    turn["actions"] = nlohmann::json::array();

    const std::array<std::pair<char, Side*>, 2> sides = {{{'a', &battle.getSideA()}, {'b', &battle.getSideB()}}};
    for (const auto& [sideName, side] : sides) {
        Pokemon* actor = side->getActivePokemon();
        Pokemon* target = (side == &battle.getSideA()) ? battle.getSideB().getActivePokemon() : battle.getSideA().getActivePokemon();
        if (!actor || actor->isFainted() || !target) {
            turn["actions"].push_back({{"side", std::string(1, sideName)}, {"type", "pass"}});
            continue;
        }

        const int hpThreshold = std::max(1, actor->getMaxHP() / 3);
        if (actor->getCurrentHP() <= hpThreshold && side->canSwitch()) {
            std::optional<int> switchIndex = chooseSwitchIndex(*side);
            if (switchIndex.has_value()) {
                turn["actions"].push_back({
                    {"side", std::string(1, sideName)},
                    {"type", "switch"},
                    {"switch_index", *switchIndex}
                });
                continue;
            }
        }

        std::optional<Move> bestMove = chooseLongBattleMove(actor, target);
        if (!bestMove.has_value()) {
            turn["actions"].push_back({{"side", std::string(1, sideName)}, {"type", "pass"}});
            continue;
        }

        turn["actions"].push_back({
            {"side", std::string(1, sideName)},
            {"type", "attack"},
            {"move_name", bestMove->getName()}
        });
    }

    return turn;
}

int countAlive(const Side& side) {
    int alive = 0;
    for (Pokemon* pokemon : side.getTeam()) {
        if (pokemon && !pokemon->isFainted()) {
            ++alive;
        }
    }
    return alive;
}
} // namespace

int main(int argc, char** argv) {
    const std::string requestPath = (argc >= 2) ? argv[1] : "tmp/meaningful_battle_active.json";

    std::ifstream input(requestPath);
    if (!input.is_open()) {
        std::cerr << "Cannot open request file: " << requestPath << std::endl;
        return 1;
    }

    nlohmann::json request = nlohmann::json::parse(input, nullptr, false);
    if (request.is_discarded()) {
        std::cerr << "Invalid JSON request" << std::endl;
        return 1;
    }

    std::string error;
    auto session = BattleSession::createFromJson(request.value("init", nlohmann::json::object()), &error);
    if (!session.has_value()) {
        std::cerr << "Init failed: " << error << std::endl;
        return 1;
    }

    Battle* battle = session->getBattle();
    if (!battle) {
        std::cerr << "Battle session did not create a battle" << std::endl;
        return 1;
    }

    int turnCount = 0;
    while (!isBattleOver(*battle) && turnCount < 50) {
        ++turnCount;

        Pokemon* activeA = battle->getSideA().getActivePokemon();
        Pokemon* activeB = battle->getSideB().getActivePokemon();
        if (activeA && activeB) {
            std::cout << "[Turn " << turnCount << "] "
                      << battle->getSideA().getName() << " active=" << activeA->getName()
                      << " HP=" << activeA->getCurrentHP() << "/" << activeA->getMaxHP()
                      << " vs "
                      << battle->getSideB().getName() << " active=" << activeB->getName()
                      << " HP=" << activeB->getCurrentHP() << "/" << activeB->getMaxHP()
                      << "\n";
        }

        nlohmann::json turnRequest = buildTurnRequest(*battle);
        if (turnRequest.contains("actions") && turnRequest["actions"].is_array()) {
            for (const auto& action : turnRequest["actions"]) {
                std::cout << "  action side=" << action.value("side", "?")
                          << " type=" << action.value("type", "?")
                          << " move=" << action.value("move_name", "-") << "\n";
            }
        }
        nlohmann::json response = session->processTurn(turnRequest);
        if (!response.value("ok", false)) {
            std::cerr << "Turn " << turnCount << " failed: " << response.dump() << std::endl;
            return 1;
        }

        const int aliveA = countAlive(battle->getSideA());
        const int aliveB = countAlive(battle->getSideB());
        std::cout << "Turn " << turnCount << ": alive A=" << aliveA << ", alive B=" << aliveB << "\n";

        if (isBattleOver(*battle)) {
            break;
        }
    }

    const int aliveA = countAlive(battle->getSideA());
    const int aliveB = countAlive(battle->getSideB());
    std::cout << "Final: turns=" << turnCount << ", alive A=" << aliveA << ", alive B=" << aliveB << "\n";
    std::cout << (aliveA == 0 ? "Winner: Blue" : "Winner: Red") << std::endl;
    return 0;
}
