cmake_minimum_required(VERSION 3.17)
project(particle_editor CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# 第三方库
add_subdirectory(third_party)

# ============================================================
# 主 exe 源文件
# ============================================================
# Classes 递归扫（将来可能有子目录）
file(GLOB_RECURSE PROJ_CLASSES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.hpp"
)

# example_sdl2_sdlrenderer2 不递归（避免扫进 CMakeFiles/ 里的临时 cpp）
file(GLOB PROJ_MAIN CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/example_sdl2_sdlrenderer2/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/example_sdl2_sdlrenderer2/*.h"
)

# Resources 递归扫（不参与编译，只让 VS 显示）
file(GLOB_RECURSE PROJ_RESOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/*"
)
set_source_files_properties(${PROJ_RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set(PROJ_SOURCES ${PROJ_CLASSES} ${PROJ_MAIN})

add_executable(${PROJECT_NAME}
    ${PROJ_SOURCES}
    ${PROJ_RESOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Classes
    ${CMAKE_CURRENT_SOURCE_DIR}/example_sdl2_sdlrenderer2
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    sdl2_libs
    imgui
    tinyxml2
    ImGuiFileDialog
)

# ============================================================
# VS 分组
# ============================================================
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             PREFIX "Source Files"
             FILES ${PROJ_SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
             PREFIX "Resources"
             FILES ${PROJ_RESOURCES})

# ============================================================
# 拷贝 SDL2 DLL
# ============================================================
file(GLOB_RECURSE SDL2_DLLS
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2/*/lib/${ARCH_DIR}/*.dll"
)
if(SDL2_DLLS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_DLLS}
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Copying SDL2 DLLs..."
    )
endif()

# 让 Resources 目录增删文件时也重跑 cmake
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
)

# ============================================================
# 资源同步（Python 增量）
# ============================================================
set(GAME_RES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
set(SYNC_SCRIPT  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/scripts/sync_folder.py")

if(EXISTS "${GAME_RES_DIR}" AND EXISTS "${SYNC_SCRIPT}")
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(NOT Python3_Interpreter_FOUND)
        find_package(Python2 COMPONENTS Interpreter REQUIRED)
        set(SYNC_PYTHON "${Python2_EXECUTABLE}")
    else()
        set(SYNC_PYTHON "${Python3_EXECUTABLE}")
    endif()

    add_custom_target(SYNC_RESOURCE ALL
        COMMAND "${SYNC_PYTHON}" "${SYNC_SCRIPT}"
            -s "${GAME_RES_DIR}"
            -d "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Resources"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Syncing Resources (incremental)..."
    )

    add_dependencies(${PROJECT_NAME} SYNC_RESOURCE)
    set_target_properties(SYNC_RESOURCE PROPERTIES FOLDER "Utils")   # ★ 放 if 里
endif()

