#include "Battle/BattleSession.h"
#include "Battle/BattleToJson.h"

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

std::optional<Move> chooseBestMove(const Pokemon* attacker, const Pokemon* defender) {
    if (!attacker) {
        return std::nullopt;
    }

    const auto& moves = attacker->getMoves();
    if (moves.empty()) {
        return std::nullopt;
    }

    std::optional<Move> bestMove;
    float bestScore = -1.0f;

    for (const Move& move : moves) {
        const int power = move.getPower();
        if (power <= 0) {
            continue;
        }

        float score = static_cast<float>(power);
        if (defender) {
            score *= defender->getTypeEffectiveness(move.getType());
        }

        if (!bestMove.has_value() || score > bestScore) {
            bestMove = move;
            bestScore = score;
        }
    }

    if (bestMove.has_value()) {
        return bestMove;
    }
    return moves.front();
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

        std::optional<Move> bestMove = chooseBestMove(actor, target);
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
        nlohmann::json turnRequest = buildTurnRequest(*battle);
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
