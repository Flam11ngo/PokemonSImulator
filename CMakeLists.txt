cmake_minimum_required(VERSION 3.14)
project(PokemonSimulator)
set(CMAKE_CXX_STANDARD 17)
include(CTest)

# Find packages
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/Battle)

# Source files
file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB SOURCES_BATTLE CONFIGURE_DEPENDS src/Battle/*.cpp)

# Ensure critical battle data sources are always linked even when CMake glob cache is stale.
list(APPEND SOURCES_BATTLE
    src/Battle/MoveData.cpp
    src/Battle/AbilityData.cpp
    src/Battle/ItemData.cpp
    src/Battle/ItemTestRunner.cpp
    src/Battle/BattleSession.cpp
)

# Battle core library (server-side reusable logic)
add_library(battle_core ${SOURCES_BATTLE})
target_include_directories(battle_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Battle
)
target_link_libraries(battle_core
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

# Executable
add_executable(PokemonSimulator ${SOURCES})

# Link libraries
target_link_libraries(PokemonSimulator
    battle_core
    nlohmann_json::nlohmann_json
)

if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(PokemonSimulatorTests
        tests/battle_data_test.cpp
    )

    target_include_directories(PokemonSimulatorTests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Battle
    )

    target_link_libraries(PokemonSimulatorTests
        battle_core
        nlohmann_json::nlohmann_json
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(PokemonSimulatorTests)
endif()


