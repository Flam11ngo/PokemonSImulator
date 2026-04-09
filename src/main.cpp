#include "Battle/BattleSession.h"
#include "Battle/Moves.h"
#include "Battle/Abilities.h"
#include "Battle/Items.h"
#include "Battle/ItemTestRunner.h"

#include <fstream>
#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

int main(int argc, char** argv){
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
                std::cerr << "Usage: --run-turn-json-files <side_a_pokemon.json> <side_b_pokemon.json> <turn.json>" << std::endl;
                return 1;
            }

            std::string error;
            auto session = BattleSession::createFromPokemonFiles(argv[2], argv[3], &error);
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
    }

    std::cout << "PokemonSimulator (server-only CLI)" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  --prefetch-moves [--refresh]" << std::endl;
    std::cout << "  --prefetch-abilities [--refresh]" << std::endl;
    std::cout << "  --prefetch-items [--refresh]" << std::endl;
    std::cout << "  --run-item-tests" << std::endl;
    std::cout << "  --run-turn-json <request.json>" << std::endl;
    std::cout << "  --run-turn-json-files <side_a_pokemon.json> <side_b_pokemon.json> <turn.json>" << std::endl;
    return 0;
}