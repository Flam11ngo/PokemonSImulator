# Battle tests
file(GLOB BATTLE_SOURCES *.cpp)
file(GLOB TEST_SOURCES ${CMAKE_SOURCE_DIR}/test/*.cpp)

# Test executable
add_executable(BattleTest ${TEST_SOURCES} ${BATTLE_SOURCES})
# Include directories for tests
target_include_directories(BattleTest PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link libraries for tests
target_link_libraries(BattleTest
    GTest::gtest_main
    ${SDL2_LIBRARIES}
    ${SDL2_GFX_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${SDL2_NET_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

# Add test
add_test(NAME BattleTest COMMAND BattleTest)