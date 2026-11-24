cmake_minimum_required(VERSION 3.17)

project(sprite_editor CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ============================================================
# ★ 1. 统一产物输出（MSVC 多配置 -> bin/Debug, bin/Release）
# ============================================================
if(CMAKE_CONFIGURATION_TYPES)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} CONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/bin/${OUTPUTCONFIG}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/lib/${OUTPUTCONFIG}")
    endforeach()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
else()
    if(CMAKE_BUILD_TYPE)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE}")
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
    endif()
endif()

# ============================================================
# ★ 2. 平台与全局编译选项
# ============================================================
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(IS_C4DROID TRUE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-fPIE)
    add_link_options(-pie)
else()
    set(IS_C4DROID FALSE)
endif()

if(MSVC)
    add_compile_options(/utf-8 /MP)
endif()

# 判断架构（用于后面拷贝 DLL）
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

# ============================================================
# 3. 收集源文件与资源
# ============================================================
file(GLOB_RECURSE PROJ_CLASSES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Classes/*.hpp"
)

file(GLOB PROJ_MAIN CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/example_sdl2_sdlrenderer2/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/example_sdl2_sdlrenderer2/*.h"
)

set(PROJ_SOURCES ${PROJ_CLASSES} ${PROJ_MAIN})

# 收集资源 (只为了在 VS 里显示，不参与编译)
set(RES_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
file(GLOB_RECURSE GAME_RESOURCES CONFIGURE_DEPENDS
    "${RES_SRC_DIR}/*.*"
)
set_source_files_properties(${GAME_RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

# ============================================================
# 4. 创建可执行文件
# ============================================================
add_executable(${PROJECT_NAME} ${PROJ_SOURCES} ${GAME_RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Classes
    ${CMAKE_CURRENT_SOURCE_DIR}/example_sdl2_sdlrenderer2
)

# ============================================================
# 5. 引入第三方库 (会生成 sdl2_libs, imgui, tinyxml2)
# ============================================================
add_subdirectory(third_party)

# ============================================================
# 6. 链接库
# ============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    sdl2_libs
    imgui
    tinyxml2
)

# ============================================================
# 7. 资源与 DLL 自动同步到目标 exe 所在目录（含 Debug / Release）
# ============================================================
if(EXISTS "${RES_SRC_DIR}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${RES_SRC_DIR}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources"
        COMMENT "Syncing Resources to $<TARGET_FILE_DIR:${PROJECT_NAME}>/Resources..."
    )
endif()

# 让资源目录增删文件时自动重跑 cmake
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RES_SRC_DIR}")

if(WIN32 AND NOT IS_C4DROID)
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
endif()

# ============================================================
# 8. VS 分组
# ============================================================
if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${PROJ_SOURCES})
    if(GAME_RESOURCES)
        source_group(TREE "${RES_SRC_DIR}" PREFIX "Resources" FILES ${GAME_RESOURCES})
    endif()
endif()