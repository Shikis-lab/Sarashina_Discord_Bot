cmake_minimum_required(VERSION 3.22)

project(sarashina_discord_bot LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(DPP_BUILD_TEST OFF CACHE BOOL "Disable D++ test targets" FORCE)
set(BUILD_VOICE_SUPPORT OFF CACHE BOOL "Disable D++ voice support" FORCE)

FetchContent_Declare(
  dpp
  GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git
  GIT_TAG v10.1.3
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(dpp nlohmann_json)

find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(sarashina_discord_bot
  src/config.cpp
  src/settings.cpp
  src/sarashina_client.cpp
  src/main.cpp
)

target_include_directories(sarashina_discord_bot PRIVATE include)

target_link_libraries(sarashina_discord_bot
  PRIVATE
    dpp
    nlohmann_json::nlohmann_json
    CURL::libcurl
    OpenSSL::Crypto
    OpenSSL::SSL
)

if (WIN32)
  target_compile_definitions(sarashina_discord_bot PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  add_custom_command(
    TARGET sarashina_discord_bot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:sarashina_discord_bot>
            $<TARGET_FILE_DIR:sarashina_discord_bot>
    COMMAND_EXPAND_LISTS
  )
endif()
