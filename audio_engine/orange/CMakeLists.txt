cmake_minimum_required(VERSION 3.17)
project(orange C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
 
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()
 
 
# ============================================================
# 先添加 external，再添加 og
# ============================================================
add_subdirectory(external)
add_subdirectory(og)
 
# ============================================================
# 链接 external / freetype 到 orange
# ============================================================
target_link_libraries(orange PUBLIC external)
 
# ============================================================
# 调试宏 / 平台宏（PUBLIC 会向下传播给所有链接 orange 的模块）
# ============================================================
target_compile_definitions(orange PUBLIC
    $<$<CONFIG:Debug>:ORANGE_DEBUG=1>
)

# Windows 平台宏（照 cocos2d 那套抄）
if(WIN32)
    target_compile_definitions(orange PUBLIC
        WIN32
        _WIN32
        _WINDOWS
        UNICODE
        _UNICODE
        _CRT_SECURE_NO_WARNINGS
        _SCL_SECURE_NO_WARNINGS
    )
endif()

# MSVC 编译选项
if(MSVC)
    target_compile_options(orange PUBLIC /MP)
endif()