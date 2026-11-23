cmake_minimum_required(VERSION 3.17)

project(particle_editor CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ============================================================
# ★ 1. 统一产物输出
# ============================================================
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/lib")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_BINARY_DIR}/lib")
endforeach()

# ============================================================
# ★ 2. 平台与全局编译选项
# ============================================================
#[[if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(IS_C4DROID TRUE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-fPIE)
    add_link_options(-pie)
else()
    set(IS_C4DROID FALSE)
endif()
]]
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
# 6. 链接库（完全统一，无需写 if-else）
# ============================================================
# 直接无脑链接 sdl2_libs，它内部会自动处理 Win .lib 或 Android -lSDL2
target_link_libraries(${PROJECT_NAME} PRIVATE
    sdl2_libs
    imgui
    tinyxml2
)

# ============================================================
# 7. 资源自动同步（兼容 VS 多配置和 C4droid 单配置）
# ============================================================
# ★ 修复：不要使用 $<TARGET_FILE_DIR>，直接指定 bin 目录，打破循环依赖
set(RES_DST_DIR "${CMAKE_BINARY_DIR}/bin/Resources")

if(EXISTS "${RES_SRC_DIR}")
    add_custom_target(SyncRes ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${RES_DST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${RES_SRC_DIR}" "${RES_DST_DIR}"
        COMMENT "Syncing Resources to bin..."
        VERBATIM
    )
    set_target_properties(SyncRes PROPERTIES FOLDER "Utils")
    # 让 exe 编译前先执行资源同步
    add_dependencies(${PROJECT_NAME} SyncRes)
endif()

# 让资源目录增删文件时自动重跑 cmake
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RES_SRC_DIR}")

# ============================================================
# 8. 自动拷贝 DLL 到 exe 旁边
# ============================================================
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
# 9. VS 分组
# ============================================================
if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${PROJ_SOURCES})
    if(GAME_RESOURCES)
        source_group(TREE "${RES_SRC_DIR}" PREFIX "Resources" FILES ${GAME_RESOURCES})
    endif()
endif()