cmake_minimum_required(VERSION 3.15)
project(roms-manager-ns VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detectar plataforma
if(PLATFORM_SWITCH)
    message(STATUS "Building for Nintendo Switch")
    set(USE_DEKO3D ON)
    set(PLATFORM_DESKTOP OFF)
    add_definitions(-D__SWITCH__ -DBOREALIS_USE_DEKO3D)
    add_definitions(-DBRLS_RESOURCES="romfs:/")
else()
    message(STATUS "Building for Desktop (PC)")
    set(PLATFORM_DESKTOP ON)
    set(USE_DEKO3D OFF)
    set(BRLS_UNITY_BUILD OFF)
    add_definitions(-DBRLS_RESOURCES="./resources/")
endif()

# Borealis
add_subdirectory(library/library)

# Sources do app
set(APP_SOURCES
    src/main.cpp
    src/main_activity.cpp
    src/debug_log.cpp
    src/views/file_browser_tab.cpp
    src/views/sync_tab.cpp
    src/views/settings_tab.cpp
    src/sync/config.cpp
    src/sync/http_client.cpp
    src/sync/sync_manager.cpp
)

# Executável
add_executable(${PROJECT_NAME} ${APP_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    library/library/include
    library/library/include/borealis/extern
)

target_link_libraries(${PROJECT_NAME} PRIVATE borealis)

# Switch: linkar libnx (ordem importa: deko3d antes de nx)
if(PLATFORM_SWITCH)
    # switch_wrapper.c contém userAppInit/userAppExit (romfsInit, socketInit, etc)
    target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/library/library/lib/platforms/switch/switch_wrapper.c
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG)
    target_link_libraries(${PROJECT_NAME} PRIVATE deko3d nx m)
endif()

# PC: symlink de resources para build dir
if(NOT PLATFORM_SWITCH)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/resources
            ${CMAKE_BINARY_DIR}/resources
        COMMENT "Linkando resources"
    )
endif()
