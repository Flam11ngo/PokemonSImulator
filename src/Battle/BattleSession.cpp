#include "Battle/BattleSession.h"

#include "Battle/BattleToJson.h"
#include "Battle/BuildFromJson.h"
#include "Battle/Items.h"
#include <cctype>
#include <fstream>
#include <optional>

namespace {

void setError(std::string* error, const std::string& message) {
    if (error) {
        *error = message;
    }
}

bool readJsonFile(const std::string& path, nlohmann::json& out, std::string* error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        setError(error, "Cannot open json file: " + path);
        return false;
    }
    out = nlohmann::json::parse(input, nullptr, false);
    if (out.is_discarded()) {
        setError(error, "Invalid json in file: " + path);
        return false;
    }
    return true;
}

bool parseSideToken(const std::string& side, bool& isA) {
    const std::string token = BattleSession::normalizeKey(side);
    if (token == "a" || token == "sidea" || token == "playera") {
        isA = true;
        return true;
    }
    if (token == "b" || token == "sideb" || token == "playerb") {
        isA = false;
        return true;
    }
    return false;
}

} // namespace

std::string BattleSession::normalizeKey(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (char ch : value) {
        if (ch == ' ' || ch == '_' || ch == '-') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

ItemType BattleSession::parseItemType(const std::string& value) {
    const std::string key = normalizeKey(value);
    for (int i = 0; i < static_cast<int>(ItemType::Count); ++i) {
        const ItemType type = static_cast<ItemType>(i);
        if (normalizeKey(getItemName(type)) == key) {
            return type;
        }
    }
    return ItemType::None;
}

std::optional<BattleSession> BattleSession::createFromPokemonFiles(const std::string& sideAPath,
                                                                   const std::string& sideBPath,
                                                                   std::string* error) {
    BattleSession session;
    try {
        Pokemon left = BuildFromJson::loadPokemonFromFile(sideAPath);
        Pokemon right = BuildFromJson::loadPokemonFromFile(sideBPath);
        session.storage.emplace_back(std::make_unique<Pokemon>(left));
        session.storage.emplace_back(std::make_unique<Pokemon>(right));

        Side sideA("Side A");
        Side sideB("Side B");
        sideA.addPokemon(session.storage[0].get());
        sideB.addPokemon(session.storage[1].get());

        session.battle = std::make_unique<Battle>(std::move(sideA), std::move(sideB));
    } catch (const std::exception& ex) {
        setError(error, ex.what());
        return std::nullopt;
    }
    return session;
}

std::optional<BattleSession> BattleSession::createFromJson(const nlohmann::json& initRequest,
                                                           std::string* error) {
    if (!initRequest.is_object()) {
        setError(error, "init request must be a json object");
        return std::nullopt;
    }

    const nlohmann::json sideAJson = initRequest.value("side_a", nlohmann::json::object());
    const nlohmann::json sideBJson = initRequest.value("side_b", nlohmann::json::object());
    if (!sideAJson.is_object() || !sideBJson.is_object()) {
        setError(error, "init request must include side_a and side_b objects");
        return std::nullopt;
    }

    const nlohmann::json teamA = sideAJson.value("pokemon", nlohmann::json::array());
    const nlohmann::json teamB = sideBJson.value("pokemon", nlohmann::json::array());
    if (!teamA.is_array() || !teamB.is_array() || teamA.empty() || teamB.empty()) {
        setError(error, "both sides must provide at least one pokemon entry");
        return std::nullopt;
    }

    BattleSession session;
    Side sideA(sideAJson.value("name", "Side A"));
    Side sideB(sideBJson.value("name", "Side B"));

    auto buildTeam = [&](const nlohmann::json& team, Side& side) -> bool {
        for (const auto& pokemonJson : team) {
            try {
                Pokemon pokemon = BuildFromJson::loadPokemonFromString(pokemonJson.dump());
                session.storage.emplace_back(std::make_unique<Pokemon>(pokemon));
                if (!side.addPokemon(session.storage.back().get())) {
                    setError(error, "failed to add pokemon into side");
                    return false;
                }
            } catch (const std::exception& ex) {
                setError(error, ex.what());
                return false;
            }
        }
        return true;
    };

    if (!buildTeam(teamA, sideA) || !buildTeam(teamB, sideB)) {
        return std::nullopt;
    }

    session.battle = std::make_unique<Battle>(std::move(sideA), std::move(sideB));
    return session;
}

Pokemon* BattleSession::selectActor(Side& side, const nlohmann::json& actionJson) {
    if (actionJson.contains("actor_index") && actionJson["actor_index"].is_number_integer()) {
        const int index = actionJson["actor_index"].get<int>();
        const auto& team = side.getTeam();
        if (index >= 0 && index < static_cast<int>(team.size())) {
            return team[index];
        }
    }
    return side.getActivePokemon();
}

Pokemon* BattleSession::selectTarget(Battle& battleRef, Side& actorSide, const nlohmann::json& actionJson) {
    Side& defaultTargetSide = (&actorSide == &battleRef.getSideA()) ? battleRef.getSideB() : battleRef.getSideA();
    Side* targetSide = &defaultTargetSide;
    if (actionJson.contains("target_side") && actionJson["target_side"].is_string()) {
        bool isA = false;
        if (parseSideToken(actionJson["target_side"].get<std::string>(), isA)) {
            targetSide = isA ? &battleRef.getSideA() : &battleRef.getSideB();
        }
    }

    if (actionJson.contains("target_index") && actionJson["target_index"].is_number_integer()) {
        const int index = actionJson["target_index"].get<int>();
        const auto& team = targetSide->getTeam();
        if (index >= 0 && index < static_cast<int>(team.size())) {
            return team[index];
        }
    }
    return targetSide->getActivePokemon();
}

nlohmann::json BattleSession::processTurn(const nlohmann::json& turnRequest) {
    if (!battle) {
        return nlohmann::json{{"ok", false}, {"error", "battle session not initialized"}};
    }
    if (!turnRequest.is_object()) {
        return nlohmann::json{{"ok", false}, {"error", "turn request must be an object"}};
    }

    const nlohmann::json actions = turnRequest.value("actions", nlohmann::json::array());
    if (!actions.is_array()) {
        return nlohmann::json{{"ok", false}, {"error", "actions must be an array"}};
    }

    bool hasA = false;
    bool hasB = false;
    std::vector<std::string> errors;

    for (const auto& actionJson : actions) {
        if (!actionJson.is_object()) {
            errors.push_back("action entry must be an object");
            continue;
        }
        bool isA = false;
        if (!actionJson.contains("side") || !actionJson["side"].is_string() ||
            !parseSideToken(actionJson["side"].get<std::string>(), isA)) {
            errors.push_back("action.side must be one of: a, b, side_a, side_b");
            continue;
        }

        Side& actorSide = isA ? battle->getSideA() : battle->getSideB();
        Side& opponentSide = isA ? battle->getSideB() : battle->getSideA();
        Pokemon* actor = selectActor(actorSide, actionJson);
        if (!actor) {
            errors.push_back("actor cannot be resolved for side " + std::string(isA ? "a" : "b"));
            continue;
        }

        const std::string type = normalizeKey(actionJson.value("type", "pass"));
        BattleAction action;

        if (type == "attack") {
            Pokemon* target = selectTarget(*battle, actorSide, actionJson);
            if (!target) {
                target = opponentSide.getActivePokemon();
            }
            const auto& moves = actor->getMoves();
            if (moves.empty()) {
                errors.push_back("actor has no moves");
                continue;
            }

            std::optional<Move> selectedMove;
            if (actionJson.contains("move_index") && actionJson["move_index"].is_number_integer()) {
                const int moveIndex = actionJson["move_index"].get<int>();
                if (moveIndex >= 0 && moveIndex < static_cast<int>(moves.size())) {
                    selectedMove = moves[moveIndex];
                }
            } else if (actionJson.contains("move_name") && actionJson["move_name"].is_string()) {
                const std::string expected = normalizeKey(actionJson["move_name"].get<std::string>());
                for (const auto& move : moves) {
                    if (normalizeKey(move.getName()) == expected) {
                        selectedMove = move;
                        break;
                    }
                }
            } else {
                selectedMove = moves.front();
            }

            if (!selectedMove.has_value()) {
                errors.push_back("attack action has invalid move selector");
                continue;
            }
            action = BattleAction::makeAttack(actor, target, selectedMove.value());
        } else if (type == "switch") {
            if (!actionJson.contains("switch_index") || !actionJson["switch_index"].is_number_integer()) {
                errors.push_back("switch action requires switch_index");
                continue;
            }
            action = BattleAction::makeSwitch(actor, actionJson["switch_index"].get<int>());
        } else if (type == "useitem" || type == "item") {
            const std::string itemName = actionJson.value("item_name", "");
            const ItemType itemType = parseItemType(itemName);
            if (itemType == ItemType::None) {
                errors.push_back("use_item action has unknown item_name");
                continue;
            }
            Pokemon* target = selectTarget(*battle, actorSide, actionJson);
            action = BattleAction::makeUseItem(actor, target, itemType);
        } else if (type == "pass") {
            action = BattleAction::makePass(actor);
        } else {
            errors.push_back("unknown action type: " + actionJson.value("type", std::string("")));
            continue;
        }

        battle->enqueueAction(action);
        if (isA) {
            hasA = true;
        } else {
            hasB = true;
        }
    }

    if (!hasA && battle->getSideA().getActivePokemon()) {
        battle->enqueueAction(BattleAction::makePass(battle->getSideA().getActivePokemon()));
    }
    if (!hasB && battle->getSideB().getActivePokemon()) {
        battle->enqueueAction(BattleAction::makePass(battle->getSideB().getActivePokemon()));
    }

    if (!errors.empty()) {
        return nlohmann::json{{"ok", false}, {"errors", errors}};
    }

    battle->processTurn();
    nlohmann::json response;
    response["ok"] = true;
    response["state"] = BattleToJson::battleToJson(*battle);
    return response;
}