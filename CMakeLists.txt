cmake_minimum_required(VERSION 3.10)
project(PokemonSimulator)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# Fetch Google Test
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(FindPkgConfig)

# Check SDL2 libraries
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_GFX REQUIRED SDL2_gfx)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(SDL2_NET REQUIRED SDL2_net)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Find packages
find_package(nlohmann_json REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_GFX_INCLUDE_DIRS})
include_directories(${SDL2_IMAGE_INCLUDE_DIRS})
include_directories(${SDL2_MIXER_INCLUDE_DIRS})
include_directories(${SDL2_NET_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
include_directories(${CURL_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/Battle)

# Source files
file(GLOB SOURCES src/*.cpp)
file(GLOB SOURCES_BATTLE src/Battle/*.cpp)
file(GLOB SOURCES_GUI src/GUI/*.cpp)
file(GLOB SOURCES_GUIUTILS src/GUIUtils/*.cpp)
file(GLOB SOURCES_SCENES src/Scenes/*.cpp)

# Executable
add_executable(PokemonSimulator ${SOURCES} ${SOURCES_BATTLE} ${SOURCES_GUI} ${SOURCES_GUIUTILS} ${SOURCES_SCENES})

# Link libraries
target_link_libraries(PokemonSimulator
    ${SDL2_LIBRARIES}
    ${SDL2_GFX_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${SDL2_NET_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    nlohmann_json::nlohmann_json
    CURL::libcurl
)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src/Battle)


