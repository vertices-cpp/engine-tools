cmake_minimum_required(VERSION 3.17)
project(audio_engine C CXX)

if(MSVC)
    add_compile_options(/utf-8 /MP)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ============================================================
# 引擎
# ============================================================
add_subdirectory(orange)

# ============================================================
# 入口 exe
# ============================================================
set(PLATFORM_SOURCE proj.win32/main.cpp)

add_executable(${PROJECT_NAME} 
#WIN32 
${PLATFORM_SOURCE})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/proj.win32
    ${CMAKE_CURRENT_SOURCE_DIR}/orange/og
    ${CMAKE_CURRENT_SOURCE_DIR}/orange/og/audio/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE orange)


# ============================================================
# 资源镜像
# ============================================================
if(EXISTS "${CMAKE_SOURCE_DIR}/Resources")
    add_custom_target(SyncRes ALL
      #[[  COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"]]
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/Resources"
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/Resources"
        COMMENT "Mirror Resources -> $<CONFIG>"
        VERBATIM)
    set_target_properties(SyncRes PROPERTIES FOLDER "Utils")

    # ★ 让 exe 依赖 SyncRes
    add_dependencies(${PROJECT_NAME} SyncRes)
endif()