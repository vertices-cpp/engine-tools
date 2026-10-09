#[[ 路径: D:/cpp/AudioEngine/test/orange/og/CMakeLists.txt ]]

cmake_minimum_required(VERSION 3.17)

set(ORANGE_VERSION 1.0)
project(orange_libs VERSION ${ORANGE_VERSION} LANGUAGES CXX)

# 1. 包含子目录里的源文件变量
include(${CMAKE_CURRENT_SOURCE_DIR}/base/CMakeLists.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/platform/CMakeLists.txt)
include(${CMAKE_CURRENT_SOURCE_DIR}/audio/CMakeLists.txt)

# 2. 汇总头文件与源文件
set(ORANGE_HEADER
    ${orange_base_header}
    ${orange_platform_header}
    ${ORANGE_AUDIO_HEADER}
)

set(ORANGE_SRC
    ${orange_base_src}
    ${orange_platform_src}
    ${ORANGE_AUDIO_SRC}
)

set(ALL_ORANGE_FILES ${ORANGE_HEADER} ${ORANGE_SRC})

# 3. 创建静态库
add_library(orange STATIC ${ALL_ORANGE_FILES})

# 4. 公开头文件搜索路径
target_include_directories(orange PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}             # 即 test/orange/og
    ${CMAKE_CURRENT_SOURCE_DIR}/base        # test/orange/og/base
    ${CMAKE_CURRENT_SOURCE_DIR}/platform    # test/orange/og/platform
    ${CMAKE_CURRENT_SOURCE_DIR}/audio       # test/orange/og/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/include
    ${CMAKE_CURRENT_SOURCE_DIR}/audio/generic
)

# 5. 输出目录与版本 
set_target_properties(orange PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    FOLDER "Internal"
)

if(MSVC OR XCODE)
    source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
                 PREFIX "Source Files"
                 FILES ${ALL_ORANGE_FILES})
endif()