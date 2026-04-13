# Metis SEC EDGAR Fraud Analyzer - CMake Build Configuration
# Version: 3.1.3
# Author: Bennie Shearer (Retired)
# Copyright (c) 2026 Bennie Shearer (Retired)
#
# DISCLAIMER: This project is NOT funded, endorsed, or approved by the
# U.S. Securities and Exchange Commission (SEC).
#
# Cross-platform C++20 build for Windows, Linux, and macOS.

cmake_minimum_required(VERSION 3.16)

# Read version from file
file(READ "${CMAKE_SOURCE_DIR}/VERSION" VERSION_STRING)
string(STRIP "${VERSION_STRING}" VERSION_STRING)

project(Metis_SEC_EDGAR_Fraud_Analyzer
    VERSION ${VERSION_STRING}
    DESCRIPTION "Metis SEC EDGAR Fraud Analyzer - Financial Forensics Tool"
    LANGUAGES CXX
)

# C++20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Generate version header
configure_file(
    "${CMAKE_SOURCE_DIR}/include/sec_analyzer/version.h.in"
    "${CMAKE_SOURCE_DIR}/include/sec_analyzer/version.h"
    @ONLY
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/http_server.cpp
    src/sec_fetcher.cpp
    src/analyzer.cpp
    src/exporter.cpp
    src/cache.cpp
    src/models/beneish.cpp
    src/models/altman.cpp
    src/models/piotroski.cpp
    src/models/fraud_triangle.cpp
    src/models/benford.cpp
)

# Header files (for IDE support)
set(HEADERS
    include/sec_analyzer/version.h
    include/sec_analyzer/types.h
    include/sec_analyzer/util.h
    include/sec_analyzer/logger.h
    include/sec_analyzer/json.h
    include/sec_analyzer/pson.h
    include/sec_analyzer/cache.h
    include/sec_analyzer/config.h
    include/sec_analyzer/http_server.h
    include/sec_analyzer/sec_fetcher.h
    include/sec_analyzer/analyzer.h
    include/sec_analyzer/exporter.h
    include/sec_analyzer/models/beneish.h
    include/sec_analyzer/models/altman.h
    include/sec_analyzer/models/piotroski.h
    include/sec_analyzer/models/fraud_triangle.h
    include/sec_analyzer/models/benford.h
)

# Create executable
add_executable(metis-sec-edgar-fraud-analyzer ${SOURCES} ${HEADERS})

# Copy client files to bin directory after build
add_custom_command(TARGET metis-sec-edgar-fraud-analyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/client"
        "${CMAKE_BINARY_DIR}/bin/client"
    COMMENT "Copying client files to bin/client"
)

# Copy images (logo, icon) into the client static directory
add_custom_command(TARGET metis-sec-edgar-fraud-analyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/images"
        "${CMAKE_BINARY_DIR}/bin/client/images"
    COMMENT "Copying images to bin/client/images"
)

# Copy config.pson next to the executable so it is found at runtime
add_custom_command(TARGET metis-sec-edgar-fraud-analyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.pson"
        "${CMAKE_BINARY_DIR}/bin/config.pson"
    COMMENT "Copying config.pson to bin/"
)

# Also copy config.pson into the client static directory so the browser
# can fetch it via the /config.pson route or as a plain static file
add_custom_command(TARGET metis-sec-edgar-fraud-analyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/config.pson"
        "${CMAKE_BINARY_DIR}/bin/client/config.pson"
    COMMENT "Copying config.pson to bin/client/"
)

# Copy docs next to the executable for reference
add_custom_command(TARGET metis-sec-edgar-fraud-analyzer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/docs"
        "${CMAKE_BINARY_DIR}/bin/docs"
    COMMENT "Copying docs to bin/docs"
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(metis-sec-edgar-fraud-analyzer ws2_32 winhttp)
    target_compile_definitions(metis-sec-edgar-fraud-analyzer PRIVATE
        _WIN32_WINNT=0x0601
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
    )
elseif(APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(metis-sec-edgar-fraud-analyzer Threads::Threads)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(metis-sec-edgar-fraud-analyzer Threads::Threads)
endif()

# Compiler-specific flags
if(MSVC)
    target_compile_options(metis-sec-edgar-fraud-analyzer PRIVATE
        /W4
        /permissive-
        /Zc:__cplusplus
        /utf-8
    )
else()
    target_compile_options(metis-sec-edgar-fraud-analyzer PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(metis-sec-edgar-fraud-analyzer PRIVATE -O3)
    endif()
endif()

# Installation
install(TARGETS metis-sec-edgar-fraud-analyzer RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/client/ DESTINATION share/metis-sec-edgar-fraud-analyzer/client)

# Print configuration summary
message(STATUS "")
message(STATUS "Metis SEC EDGAR Fraud Analyzer v${PROJECT_VERSION}")
message(STATUS "Author: Bennie Shearer (Retired)")
message(STATUS "Copyright (c) 2026 Bennie Shearer (Retired)")
message(STATUS "")
message(STATUS "Configuration:")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "")
