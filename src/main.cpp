#include "IO/BattleSession.h"
#include "IO/BattleToJson.h"
#include "battle/GameRegistry.h"
#include "battle/Moves.h"
#include "battle/Abilities.h"
#include "battle/Items.h"
#include "tests/ItemTestRunner.h"
#include "tests/MoveTestRunner.h"

#include <fstream>
#include <iostream>
#include <cstdint>
#include <filesystem>
#include <map>
#include <regex>
#include <string>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

bool readJsonFromFile(const std::string& path, nlohmann::json& out, std::string& error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        error = "Cannot open json file: " + path;
        return false;
    }
    out = nlohmann::json::parse(input, nullptr, false);
    if (out.is_discarded()) {
        error = "Invalid json in file: " + path;
        return false;
    }
    return true;
}

bool loadInitFromCacheInput(nlohmann::json& initJson, std::string& error) {
    const std::vector<std::string> candidates = {
        "cache/input/init_request.json",
        "cache/input/init.json",
    };
    for (const auto& path : candidates) {
        if (!std::filesystem::exists(path)) {
            continue;
        }
        if (readJsonFromFile(path, initJson, error)) {
            return true;
        }
        return false;
    }
    // Fallback: try side_a.json + side_b.json
    const std::string sideAPath = "cache/input/side_a.json";
    const std::string sideBPath = "cache/input/side_b.json";
    if (std::filesystem::exists(sideAPath) && std::filesystem::exists(sideBPath)) {
        nlohmann::json sideA, sideB;
        std::string parseError;
        if (!readJsonFromFile(sideAPath, sideA, parseError)) {
            error = "Failed to parse cache/input/side_a.json: " + parseError;
            return false;
        }
        if (!readJsonFromFile(sideBPath, sideB, parseError)) {
            error = "Failed to parse cache/input/side_b.json: " + parseError;
            return false;
        }
        initJson = nlohmann::json::object();
        initJson["side_a"] = sideA.is_object() ? sideA : nlohmann::json::object();
        initJson["side_b"] = sideB.is_object() ? sideB : nlohmann::json::object();
        if (!initJson["side_a"].contains("name")) {
            initJson["side_a"]["name"] = "Side A";
        }
        if (!initJson["side_b"].contains("name")) {
            initJson["side_b"]["name"] = "Side B";
        }
        return true;
    }

    error = "Missing init json in cache/input (expected init_request.json, init.json, or side_a.json + side_b.json)";
    return false;
}

nlohmann::json normalizeSideAction(const nlohmann::json& actionInput, const char* sideToken) {
    if (!actionInput.is_object()) {
        return nlohmann::json{{"side", sideToken}, {"type", "pass"}};
    }
    nlohmann::json action = actionInput;
    if (!action.contains("side")) {
        action["side"] = sideToken;
    }
    if (action.contains("type") && action["type"].is_string()) {
        const std::string type = action["type"].get<std::string>();
        if ((type == "switch" || type == "Switch") && !action.contains("switch_index")
            && action.contains("target_index") && action["target_index"].is_number_integer()) {
            action["switch_index"] = action["target_index"];
        }
    }
    return action;
}

bool runCacheInputBattle() {
    namespace fs = std::filesystem;

    nlohmann::json initJson;
    std::string error;
    if (!loadInitFromCacheInput(initJson, error)) {
        std::cerr << "Init failed: " << error << std::endl;
        return false;
    }

    std::map<int, std::pair<nlohmann::json, nlohmann::json>> turnInputs;
    const std::regex inputPattern(R"(^([12])_input_(\d+)\.json$)");
    const fs::path inputDir("cache/input");
    if (fs::exists(inputDir) && fs::is_directory(inputDir)) {
        for (const auto& entry : fs::directory_iterator(inputDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::string filename = entry.path().filename().string();
            std::smatch match;
            if (!std::regex_match(filename, match, inputPattern)) {
                continue;
            }

            const int sideNumber = std::stoi(match[1].str());
            const int turnNumber = std::stoi(match[2].str());
            nlohmann::json actionJson;
            std::string parseError;
            if (!readJsonFromFile(entry.path().string(), actionJson, parseError)) {
                std::cerr << "Input parse failed: " << parseError << std::endl;
                return false;
            }

            if (sideNumber == 1) {
                turnInputs[turnNumber].first = std::move(actionJson);
            } else {
                turnInputs[turnNumber].second = std::move(actionJson);
            }
        }
    }

    std::string sessionError;
    auto session = BattleSession::createFromJson(initJson, &sessionError);
    if (!session.has_value()) {
        std::cerr << "Init failed: " << sessionError << std::endl;
        return false;
    }

    for (const auto& [turnNumber, sideInputs] : turnInputs) {
        nlohmann::json turnRequest{
            {"actions", nlohmann::json::array({
                normalizeSideAction(sideInputs.first, "a"),
                normalizeSideAction(sideInputs.second, "b")
            })}
        };
        const nlohmann::json response = session->processTurn(turnRequest);
        if (!response.value("ok", false)) {
            std::cerr << "Turn " << turnNumber << " failed: " << response.dump(2) << std::endl;
            return false;
        }

        const Battle* battle = session->getBattle();
        if (!battle) {
            std::cerr << "Battle session lost after turn " << turnNumber << std::endl;
            return false;
        }
        if (!battle->getSideA().hasRemainingPokemon() || !battle->getSideB().hasRemainingPokemon()) {
            break;
        }
    }

    Battle* battle = session->getBattle();
    if (!battle) {
        std::cerr << "No battle result available" << std::endl;
        return false;
    }

    std::ifstream finalOutput("cache/output/output_" + std::to_string(battle->getTurnNumber()) + ".json");
    if (finalOutput.is_open()) {
        nlohmann::json out = nlohmann::json::parse(finalOutput, nullptr, false);
        if (!out.is_discarded()) {
            std::cout << out.dump(2) << std::endl;
            return true;
        }
    }

    std::cout << BattleToJson::battleAllInfoToJson(*battle).dump(2) << std::endl;
    return true;
}

bool runDaemonMode() {
    namespace fs = std::filesystem;
    std::cout << "[daemon] starting, waiting for init files in cache/input/..." << std::endl;

    // Phase A: wait for init files
    nlohmann::json initJson;
    std::string error;
    while (true) {
        if (loadInitFromCacheInput(initJson, error)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "[daemon] init loaded" << std::endl;

    auto session = BattleSession::createDeferred(initJson, &error);
    if (!session.has_value()) {
        std::cerr << "[daemon] init failed: " << error << std::endl;
        return false;
    }

    // Phase B: wait for turn 0 inputs (switch to active pokemon)
    std::cout << "[daemon] waiting for turn 0 inputs (1_input_0.json, 2_input_0.json)..." << std::endl;
    nlohmann::json sideAInput0;
    nlohmann::json sideBInput0;
    while (true) {
        const std::string pathA = "cache/input/1_input_0.json";
        const std::string pathB = "cache/input/2_input_0.json";
        if (fs::exists(pathA) && fs::exists(pathB)) {
            std::string parseError;
            if (!readJsonFromFile(pathA, sideAInput0, parseError)) {
                std::cerr << "[daemon] failed to parse 1_input_0.json: " << parseError << std::endl;
                return false;
            }
            if (!readJsonFromFile(pathB, sideBInput0, parseError)) {
                std::cerr << "[daemon] failed to parse 2_input_0.json: " << parseError << std::endl;
                return false;
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Process turn 0 switches
    Battle* battle = session->getBattle();
    auto processTurn0Switch = [&](Side& side, const nlohmann::json& input, const char* label) {
        std::string type = BattleSession::normalizeKey(input.value("type", "pass"));
        if (type == "switch") {
            int switchIndex = -1;
            if (input.contains("switch_index") && input["switch_index"].is_number_integer()) {
                switchIndex = input["switch_index"].get<int>();
            } else if (input.contains("target_index") && input["target_index"].is_number_integer()) {
                switchIndex = input["target_index"].get<int>();
            }
            if (switchIndex >= 0 && switchIndex < side.getPokemonCount()
                && switchIndex != side.getActiveIndex()) {
                side.switchActive(switchIndex);
                std::cout << "[daemon] turn 0: " << label << " switched to slot "
                          << switchIndex << std::endl;
            }
        }
    };
    processTurn0Switch(battle->getSideA(), sideAInput0, "side A");
    processTurn0Switch(battle->getSideB(), sideBInput0, "side B");

    session->doInitialSendOut();
    std::cout << "[daemon] turn 0 done, output_0.json written" << std::endl;

    // Phase C: main turn loop
    int turnNumber = 0;
    while (true) {
        ++turnNumber;
        const std::string pathA = "cache/input/1_input_" + std::to_string(turnNumber) + ".json";
        const std::string pathB = "cache/input/2_input_" + std::to_string(turnNumber) + ".json";

        std::cout << "[daemon] waiting for turn " << turnNumber
                  << " inputs..." << std::endl;
        while (true) {
            if (fs::exists(pathA) && fs::exists(pathB)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        nlohmann::json sideAAction;
        nlohmann::json sideBAction;
        std::string parseError;
        if (!readJsonFromFile(pathA, sideAAction, parseError)) {
            std::cerr << "[daemon] failed to parse " << pathA << ": " << parseError << std::endl;
            return false;
        }
        if (!readJsonFromFile(pathB, sideBAction, parseError)) {
            std::cerr << "[daemon] failed to parse " << pathB << ": " << parseError << std::endl;
            return false;
        }

        nlohmann::json turnRequest{
            {"actions", nlohmann::json::array({
                normalizeSideAction(sideAAction, "a"),
                normalizeSideAction(sideBAction, "b")
            })}
        };
        const nlohmann::json response = session->processTurn(turnRequest);
        if (!response.value("ok", false)) {
            std::cerr << "[daemon] turn " << turnNumber << " failed: "
                      << response.dump(2) << std::endl;
            return false;
        }

        if (!battle) {
            std::cerr << "[daemon] battle session lost after turn " << turnNumber << std::endl;
            return false;
        }
        if (!battle->getSideA().hasRemainingPokemon() || !battle->getSideB().hasRemainingPokemon()) {
            std::cout << "[daemon] battle ended after turn " << turnNumber << std::endl;
            nlohmann::json gameOver;
            gameOver["game_over"] = true;
            gameOver["winner"] = battle->getSideA().hasRemainingPokemon()
                                     ? battle->getSideA().getName()
                                     : battle->getSideB().getName();
            gameOver["final_turn"] = turnNumber;
            std::ofstream go("cache/output/game_over.json");
            if (go.is_open()) {
                go << gameOver.dump(2);
            }
            break;
        }

        std::cout << "[daemon] turn " << turnNumber << " done, output_"
                  << battle->getTurnNumber() << ".json written" << std::endl;
    }

    return true;
}

} // namespace

int main(int argc, char** argv){
    GameRegistry::instance().init();

    if (argc >= 2) {
        const std::string arg = argv[1];
        if (arg == "--prefetch-moves") {
            const bool refresh = (argc >= 3 && std::string(argv[2]) == "--refresh");
            const bool ok = prefetchMovesFromPokeAPI(refresh);
            return ok ? 0 : 1;
        }
        if (arg == "--prefetch-abilities") {
            const bool refresh = (argc >= 3 && std::string(argv[2]) == "--refresh");
            const bool ok = prefetchAbilitiesFromPokeAPI(refresh);
            return ok ? 0 : 1;
        }
        if (arg == "--prefetch-items") {
            const bool refresh = (argc >= 3 && std::string(argv[2]) == "--refresh");
            const bool ok = prefetchItemsFromPokeAPI(refresh);
            return ok ? 0 : 1;
        }
        if (arg == "--run-item-tests") {
            const ItemTestSummary summary = runAllItemTests(std::cout, std::cerr);
            if (!summary.failedItemNames.empty()) {
                std::cerr << "Failed items:" << std::endl;
                for (const auto& itemName : summary.failedItemNames) {
                    std::cerr << "  - " << itemName << std::endl;
                }
            }
            return summary.failed == 0 ? 0 : 1;
        }
        if (arg == "--run-move-tests") {
            const MoveTestSummary summary = runAllMoveTests(std::cout, std::cerr);
            if (!summary.failedMoveNames.empty()) {
                std::cerr << "Failed moves:" << std::endl;
                for (const auto& moveName : summary.failedMoveNames) {
                    std::cerr << "  - " << moveName << std::endl;
                }
            }
            if (!summary.unsupportedStatusMoveNames.empty()) {
                std::cerr << "Unsupported status moves:" << std::endl;
                for (const auto& moveName : summary.unsupportedStatusMoveNames) {
                    std::cerr << "  - " << moveName << std::endl;
                }
            }
            return summary.failed == 0 ? 0 : 1;
        }
        if (arg == "--run-turn-json") {
            if (argc < 3) {
                std::cerr << "Usage: --run-turn-json <request.json>" << std::endl;
                return 1;
            }

            std::ifstream input(argv[2]);
            if (!input.is_open()) {
                std::cerr << "Cannot open request file: " << argv[2] << std::endl;
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

            const nlohmann::json response = session->processTurn(request.value("turn", nlohmann::json::object()));
            std::cout << response.dump(2) << std::endl;
            return response.value("ok", false) ? 0 : 1;
        }
        if (arg == "--run-turn-json-files") {
            if (argc < 5) {
                std::cerr << "Usage: --run-turn-json-files <side_a_pokemon.json> <side_b_pokemon.json> <turn.json> [seed]" << std::endl;
                return 1;
            }

            std::string error;
            uint32_t seed = 0;
            if (argc >= 6) {
                try {
                    seed = static_cast<uint32_t>(std::stoul(argv[5]));
                } catch (...) {
                    std::cerr << "Invalid seed: " << argv[5] << std::endl;
                    return 1;
                }
            }

            auto session = BattleSession::createFromPokemonFiles(argv[2], argv[3], seed, &error);
            if (!session.has_value()) {
                std::cerr << "Init failed: " << error << std::endl;
                return 1;
            }

            nlohmann::json turnRequest;
            std::ifstream turnInput(argv[4]);
            if (!turnInput.is_open()) {
                std::cerr << "Cannot open turn file: " << argv[4] << std::endl;
                return 1;
            } else {
                turnRequest = nlohmann::json::parse(turnInput, nullptr, false);
            }

            if (turnRequest.is_discarded()) {
                std::cerr << "Invalid turn JSON" << std::endl;
                return 1;
            }

            const nlohmann::json response = session->processTurn(turnRequest);
            std::cout << response.dump(2) << std::endl;
            return response.value("ok", false) ? 0 : 1;
        }
        if (arg == "--run-cache-input") {
            return runCacheInputBattle() ? 0 : 1;
        }
        if (arg == "--daemon") {
            return runDaemonMode() ? 0 : 1;
        }
    }

    std::cout << "PokemonSimulator (server-only CLI)" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  --prefetch-moves [--refresh]" << std::endl;
    std::cout << "  --prefetch-abilities [--refresh]" << std::endl;
    std::cout << "  --prefetch-items [--refresh]" << std::endl;
    std::cout << "  --run-item-tests" << std::endl;
    std::cout << "  --run-move-tests" << std::endl;
    std::cout << "  --run-turn-json <request.json>" << std::endl;
    std::cout << "  --run-turn-json-files <side_a_pokemon.json> <side_b_pokemon.json> <turn.json> [seed]" << std::endl;
    std::cout << "  --run-cache-input" << std::endl;
    std::cout << "  --daemon" << std::endl;
    return 0;
}
