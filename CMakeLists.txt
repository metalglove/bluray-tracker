cmake_minimum_required(VERSION 3.22)

project(BluRayTracker
    VERSION 1.0.0
    DESCRIPTION "Personal Blu-ray / UHD 4K movie & player tracker"
    LANGUAGES CXX
)

# C++17 requirement
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# System dependencies (must be installed)
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Find gumbo-parser (system package)
find_library(GUMBO_LIBRARY NAMES gumbo)
find_path(GUMBO_INCLUDE_DIR NAMES gumbo.h)
if(NOT GUMBO_LIBRARY OR NOT GUMBO_INCLUDE_DIR)
    message(FATAL_ERROR "gumbo-parser not found. Install libgumbo-dev")
endif()

# FetchContent for header-only dependencies
include(FetchContent)

# Asio (standalone)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
)
FetchContent_MakeAvailable(asio)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include CACHE PATH "Asio include dir" FORCE)
message(STATUS "ASIO INCLUDE DIR: ${ASIO_INCLUDE_DIR}")

# Crow (web framework)
FetchContent_Declare(
    Crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG        v1.2.0
)

set(CROW_USE_BOOST OFF)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

# Make dependencies available
# Make dependencies available
FetchContent_MakeAvailable(Crow json)

# fmt library (replacement for std::format in C++17)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        10.2.1
)
FetchContent_MakeAvailable(fmt)

# Source files
set(SOURCES
    src/main.cpp
    src/domain/models.cpp
    src/infrastructure/logger.cpp
    src/infrastructure/database_manager.cpp
    src/infrastructure/config_manager.cpp
    src/infrastructure/network_client.cpp
    src/infrastructure/image_cache.cpp
    src/infrastructure/tmdb_client.cpp
    src/infrastructure/repositories/wishlist_repository.cpp
    src/infrastructure/repositories/collection_repository.cpp
    src/infrastructure/repositories/release_calendar_repository.cpp
    src/infrastructure/repositories/price_history_repository.cpp
    src/infrastructure/repositories/tag_repository.cpp
    src/infrastructure/repositories/deal_repository.cpp
    src/application/scraper/scraper.cpp
    src/application/scraper/amazon_nl_scraper.cpp
    src/application/scraper/bol_com_scraper.cpp
    src/application/scraper/bluray_com_scraper.cpp
    src/application/enrichment/tmdb_enrichment_service.cpp
    src/application/notifier/discord_notifier.cpp
    src/application/notifier/email_notifier.cpp
    src/application/deals/deals_service.cpp
    src/application/scheduler.cpp
    src/presentation/web_frontend.cpp
    src/presentation/html_renderer.cpp
)

# Main executable
add_executable(bluray-tracker ${SOURCES})

# Include directories
target_include_directories(bluray-tracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${GUMBO_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(bluray-tracker PRIVATE
    Crow::Crow
    nlohmann_json::nlohmann_json
    CURL::libcurl
    SQLite::SQLite3
    OpenSSL::Crypto
    ${GUMBO_LIBRARY}
    Threads::Threads
    fmt::fmt
)

# Install target
install(TARGETS bluray-tracker
    RUNTIME DESTINATION bin
)

# Create cache directory
install(DIRECTORY DESTINATION cache)
