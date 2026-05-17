#include "IO/BattleSession.h"

#include "IO/BattleToJson.h"
#include "IO/BuildFromJson.h"
#include "battle/Items.h"
#include "battle/PRNG.h"
#include <cctype>
#include <filesystem>
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

void writeInputCache(const nlohmann::json& data, const std::string& filename) {
    std::filesystem::create_directories("cache/input");
    std::ofstream output("cache/input/" + filename);
    if (!output.is_open()) {
        return;
    }
    output << data.dump(2);
}

void resetCacheFolders() {
    std::filesystem::remove_all("cache/input");
    std::filesystem::remove_all("cache/output");
    std::filesystem::create_directories("cache/input");
    std::filesystem::create_directories("cache/output");
}

nlohmann::json collectActions(const nlohmann::json& turnRequest);

void splitSideInputs(const nlohmann::json& turnRequest, nlohmann::json& sideAInput, nlohmann::json& sideBInput) {
    sideAInput = nlohmann::json{{"type", "pass"}};
    sideBInput = nlohmann::json{{"type", "pass"}};

    const nlohmann::json actions = collectActions(turnRequest);
    if (actions.is_array()) {
        for (const auto& actionJson : actions) {
            if (!actionJson.is_object()) {
                continue;
            }
            bool isA = false;
            if (!actionJson.contains("side") || !actionJson["side"].is_string()
                || !parseSideToken(actionJson["side"].get<std::string>(), isA)) {
                continue;
            }
            if (isA && sideAInput.value("type", std::string("pass")) == "pass") {
                sideAInput = actionJson;
            }
            if (!isA && sideBInput.value("type", std::string("pass")) == "pass") {
                sideBInput = actionJson;
            }
        }
    }
}

nlohmann::json collectActions(const nlohmann::json& turnRequest) {
    if (turnRequest.contains("actions")) {
        return turnRequest["actions"];
    }

    nlohmann::json actions = nlohmann::json::array();
    auto appendSideAction = [&](const char* key, const char* sideToken) {
        if (!turnRequest.contains(key) || !turnRequest[key].is_object()) {
            return;
        }
        nlohmann::json action = turnRequest[key];
        if (!action.contains("side")) {
            action["side"] = sideToken;
        }
        actions.push_back(std::move(action));
    };

    appendSideAction("side_a", "a");
    appendSideAction("side_b", "b");
    appendSideAction("a", "a");
    appendSideAction("b", "b");
    appendSideAction("player_a", "a");
    appendSideAction("player_b", "b");
    appendSideAction("side_a_action", "a");
    appendSideAction("side_b_action", "b");
    appendSideAction("action_a", "a");
    appendSideAction("action_b", "b");

    if (actions.empty() && turnRequest.contains("side") && turnRequest["side"].is_string()) {
        actions.push_back(turnRequest);
    }
    return actions;
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
                                                                   uint32_t seed,
                                                                   std::string* error) {
    BattleSession session;
    try {
        resetCacheFolders();
        if (seed != 0) {
            PRNG::setSeed(seed);
        }
        std::vector<Pokemon> teamA = BuildFromJson::loadPokemonTeamFromFile(sideAPath);
        std::vector<Pokemon> teamB = BuildFromJson::loadPokemonTeamFromFile(sideBPath);
        if (teamA.empty() || teamB.empty()) {
            setError(error, "each side must have at least one pokemon");
            return std::nullopt;
        }

        Side sideA("Side A");
        Side sideB("Side B");
        for (auto& p : teamA) {
            session.storage.emplace_back(std::make_unique<Pokemon>(std::move(p)));
            sideA.addPokemon(session.storage.back().get());
        }
        for (auto& p : teamB) {
            session.storage.emplace_back(std::make_unique<Pokemon>(std::move(p)));
            sideB.addPokemon(session.storage.back().get());
        }

        session.battle = std::make_unique<Battle>(std::move(sideA), std::move(sideB));
        BattleToJson::writeToCache(BattleToJson::battleAllInfoToJson(*session.battle), "output_0.json");
    } catch (const std::exception& ex) {
        setError(error, ex.what());
        return std::nullopt;
    }
    return session;
}

std::optional<BattleSession> BattleSession::createFromJson(const nlohmann::json& initRequest,
                                                           std::string* error) {
    resetCacheFolders();
    writeInputCache(initRequest, "init_request.json");
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

    if (initRequest.contains("seed")) {
        if (!initRequest["seed"].is_number_integer()) {
            setError(error, "seed must be an integer when provided");
            return std::nullopt;
        }
        PRNG::setSeed(static_cast<uint32_t>(initRequest["seed"].get<int64_t>()));
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
    BattleToJson::writeToCache(BattleToJson::battleAllInfoToJson(*session.battle), "output_0.json");
    return session;
}

std::optional<BattleSession> BattleSession::createDeferred(const nlohmann::json& initRequest,
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

    if (initRequest.contains("seed")) {
        if (!initRequest["seed"].is_number_integer()) {
            setError(error, "seed must be an integer when provided");
            return std::nullopt;
        }
        PRNG::setSeed(static_cast<uint32_t>(initRequest["seed"].get<int64_t>()));
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

    session.battle = std::make_unique<Battle>(std::move(sideA), std::move(sideB),
                                              GameRegistry::instance(), false);
    return session;
}

void BattleSession::doInitialSendOut() {
    if (!battle) return;

    Pokemon* activeA = battle->getSideA().getActivePokemon();
    Pokemon* activeB = battle->getSideB().getActivePokemon();

    if (activeA) {
        battle->appendSpecialEvent("switch_in", {
            {"side", battle->getSideA().getName()},
            {"pokemon", activeA->getName()},
            {"reason", "initial_send_out"}
        });
    }
    if (activeB) {
        battle->appendSpecialEvent("switch_in", {
            {"side", battle->getSideB().getName()},
            {"pokemon", activeB->getName()},
            {"reason", "initial_send_out"}
        });
    }

    if (activeA) {
        battle->triggerAbility(activeA, Trigger::OnEntry, activeB);
        battle->triggerItemEffect(activeA, ItemTrigger::OnEntry, activeB);
    }
    if (activeB) {
        battle->triggerAbility(activeB, Trigger::OnEntry, activeA);
        battle->triggerItemEffect(activeB, ItemTrigger::OnEntry, activeA);
    }

    BattleToJson::writeToCache(BattleToJson::battleAllInfoToJson(*battle), "output_0.json");
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

    // 清理上回合累积事件，确保每回合输出仅包含本回合事件
    battle->clearSpecialEvents();

    const int nextTurn = battle->getTurnNumber() + 1;
    nlohmann::json sideAInput;
    nlohmann::json sideBInput;
    splitSideInputs(turnRequest, sideAInput, sideBInput);
    writeInputCache(sideAInput, "1_input_" + std::to_string(nextTurn) + ".json");
    writeInputCache(sideBInput, "2_input_" + std::to_string(nextTurn) + ".json");

    const nlohmann::json actions = collectActions(turnRequest);
    if (!actions.is_array()) {
        return nlohmann::json{{"ok", false}, {"error", "actions must be an array"}};
    }
    if (actions.size() > 2) {
        return nlohmann::json{{"ok", false}, {"error", "singles mode supports at most one action per side"}};
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
        if ((isA && hasA) || (!isA && hasB)) {
            errors.push_back("duplicate action for side " + std::string(isA ? "a" : "b") + " in singles turn");
            continue;
        }

        Side& actorSide = isA ? battle->getSideA() : battle->getSideB();
        Side& opponentSide = isA ? battle->getSideB() : battle->getSideA();
        Pokemon* actor = selectActor(actorSide, actionJson);
        if (!actor) {
            errors.push_back("actor cannot be resolved for side " + std::string(isA ? "a" : "b"));
            continue;
        }
        if (actionJson.contains("actor_index") && actionJson["actor_index"].is_number_integer()) {
            const int actorIndex = actionJson["actor_index"].get<int>();
            if (actorIndex < 0 || actorIndex >= actorSide.getPokemonCount()) {
                errors.push_back("actor_index is out of range for side " + std::string(isA ? "a" : "b"));
                continue;
            }
            if (actor != actorSide.getActivePokemon()) {
                errors.push_back("actor_index must reference the active pokemon in singles mode");
                continue;
            }
        } else if (actionJson.contains("actor_index")) {
            errors.push_back("actor_index must be an integer");
            continue;
        }

        const std::string type = normalizeKey(actionJson.value("type", "pass"));
        BattleAction action;

        if (type == "attack" || type == "move") {
            if (actionJson.contains("move_index") && actionJson.contains("move_name")) {
                errors.push_back("attack action cannot specify both move_index and move_name");
                continue;
            }
            if (actionJson.contains("target_side") && !actionJson["target_side"].is_string()) {
                errors.push_back("target_side must be a string when provided");
                continue;
            }
            if (actionJson.contains("target_side") && actionJson["target_side"].is_string()) {
                bool parsedTargetSide = false;
                if (!parseSideToken(actionJson["target_side"].get<std::string>(), parsedTargetSide)) {
                    errors.push_back("target_side must be one of: a, b, side_a, side_b");
                    continue;
                }
            }
            if (actionJson.contains("target_index") && !actionJson["target_index"].is_number_integer()) {
                errors.push_back("target_index must be an integer when provided");
                continue;
            }

            Pokemon* target = selectTarget(*battle, actorSide, actionJson);
            if (!target) {
                target = opponentSide.getActivePokemon();
            }
            if (!target) {
                errors.push_back("attack action target cannot be resolved");
                continue;
            }

            if (actionJson.contains("target_index") && actionJson["target_index"].is_number_integer()) {
                const int targetIndex = actionJson["target_index"].get<int>();
                Side* selectedSide = &opponentSide;
                if (actionJson.contains("target_side") && actionJson["target_side"].is_string()) {
                    bool targetIsA = false;
                    parseSideToken(actionJson["target_side"].get<std::string>(), targetIsA);
                    selectedSide = targetIsA ? &battle->getSideA() : &battle->getSideB();
                }
                if (targetIndex < 0 || targetIndex >= selectedSide->getPokemonCount()) {
                    errors.push_back("target_index is out of range for selected target_side");
                    continue;
                }
            }

            const auto& moves = actor->getMoves();
            if (moves.empty()) {
                errors.push_back("actor has no moves");
                continue;
            }

            std::optional<Move> selectedMove;
            if (actionJson.contains("move_index") && !actionJson["move_index"].is_number_integer()) {
                errors.push_back("move_index must be an integer when provided");
                continue;
            }
            if (actionJson.contains("move_name") && !actionJson["move_name"].is_string()) {
                errors.push_back("move_name must be a string when provided");
                continue;
            }

            if (actionJson.contains("move_index") && actionJson["move_index"].is_number_integer()) {
                const int moveIndex = actionJson["move_index"].get<int>();
                if (moveIndex >= 0 && moveIndex < static_cast<int>(moves.size())) {
                    selectedMove = moves[moveIndex];
                } else {
                    errors.push_back("attack action move_index is out of range");
                    continue;
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
            int switchIndex = -1;
            if (actionJson.contains("switch_index")) {
                if (!actionJson["switch_index"].is_number_integer()) {
                    errors.push_back("switch_index must be an integer when provided");
                    continue;
                }
                switchIndex = actionJson["switch_index"].get<int>();
            } else if (actionJson.contains("target_index")) {
                if (!actionJson["target_index"].is_number_integer()) {
                    errors.push_back("target_index must be an integer when provided for switch action");
                    continue;
                }
                switchIndex = actionJson["target_index"].get<int>();
            } else {
                errors.push_back("switch action requires switch_index (or target_index)");
                continue;
            }
            if (switchIndex < 0 || switchIndex >= actorSide.getPokemonCount()) {
                errors.push_back("switch_index is out of range for side " + std::string(isA ? "a" : "b"));
                continue;
            }
            if (switchIndex == actorSide.getActiveIndex()) {
                errors.push_back("switch_index cannot target currently active pokemon");
                continue;
            }
            const Pokemon* switchTarget = actorSide.getTeam()[switchIndex];
            if (!switchTarget || switchTarget->isFainted()) {
                errors.push_back("switch_index must reference a non-fainted pokemon");
                continue;
            }
            action = BattleAction::makeSwitch(actor, switchIndex);
        } else if (type == "useitem" || type == "item") {
            if (!actionJson.contains("item_name") || !actionJson["item_name"].is_string()) {
                errors.push_back("use_item action requires string item_name");
                continue;
            }
            if (actionJson.contains("target_side") && !actionJson["target_side"].is_string()) {
                errors.push_back("target_side must be a string when provided");
                continue;
            }
            if (actionJson.contains("target_index") && !actionJson["target_index"].is_number_integer()) {
                errors.push_back("target_index must be an integer when provided");
                continue;
            }
            const std::string itemName = actionJson.value("item_name", "");
            const ItemType itemType = parseItemType(itemName);
            if (itemType == ItemType::None) {
                errors.push_back("use_item action has unknown item_name");
                continue;
            }
            Pokemon* target = selectTarget(*battle, actorSide, actionJson);
            if (!target) {
                errors.push_back("use_item action target cannot be resolved");
                continue;
            }
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
    const nlohmann::json battleAllInfo = BattleToJson::battleAllInfoToJson(*battle);
    BattleToJson::writeToCache(battleAllInfo, "output_" + std::to_string(battle->getTurnNumber()) + ".json");

    nlohmann::json response;
    response["ok"] = true;
    response["state"] = BattleToJson::battleToJson(*battle);
    response["battle_all_info"] = battleAllInfo;
    response["description"] = battleAllInfo.value("descriptions", nlohmann::json::array());
    response["battle"] = battleAllInfo.value("battle", nlohmann::json::object());
    return response;
}
