cmake_minimum_required(VERSION 3.17)
project(external)

# 源码库
add_subdirectory(zlib) 

# ogg
add_subdirectory(ogg)

# vorbis
set(OGG_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ogg/include")
set(OGG_LIBRARY ogg)
add_subdirectory(vorbis)

# mpg123
set(BUILD_PROGRAMS   OFF CACHE BOOL "" FORCE)
set(BUILD_LIBOUT123  OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING    OFF CACHE BOOL "" FORCE)
add_subdirectory(mpg123-1.33.7/ports/cmake)





# openal
set(ALSOFT_UTILS     OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL   OFF CACHE BOOL "" FORCE)

add_subdirectory(openal-soft-1.22.0)

# C4droid 环境：CMAKE_SYSTEM_NAME=Linux, CMAKE_SYSTEM_PROCESSOR=aarch64, ANDROID 变量为空
if(NOT WIN32 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(IS_ANDROID_ENV TRUE)
else()
    set(IS_ANDROID_ENV FALSE)
endif()

message(STATUS "IS_ANDROID_ENV = ${IS_ANDROID_ENV}")

add_library(external INTERFACE)
target_link_libraries(external INTERFACE
 
    zlib 
    ogg
    vorbis
    vorbisfile
    libmpg123
    OpenAL
)

# Android 上把 OpenSLES  也传下去
if(IS_ANDROID_ENV)
    target_link_libraries(external INTERFACE OpenSLES )
endif()


# ★ 手动加 mpg123 的 include 路径
target_include_directories(external INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/mpg123-1.33.7/src/include"
)

foreach(tgt 
    zlib 
    ogg
    vorbis
    vorbisfile
    compat
    compat_dl
    compat_str
    libmpg123
    libsyn123
    OpenAL
    common
)
    if(TARGET ${tgt})
        set_target_properties(${tgt} PROPERTIES FOLDER "External")
    endif()
endforeach()
