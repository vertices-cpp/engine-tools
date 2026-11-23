# ============================================================
# 1. 平台环境判断
# ============================================================
# 如果子库需要 PIE（C4droid / Linux 上必须）
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_compile_options(-fPIE)
endif()

if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(IS_C4DROID TRUE)
else()
    set(IS_C4DROID FALSE)
endif()

# ============================================================
# 2. 统一封装 SDL2 依赖库 (sdl2_libs)
# ============================================================
# 无论什么平台，先建好 INTERFACE 库，这样主工程和 imgui 闭着眼睛链都不会报错
add_library(sdl2_libs INTERFACE)

# 统一使用本地的头文件，保证跨平台头文件版本一致
target_include_directories(sdl2_libs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2-2.0.20/include
)

if(NOT IS_C4DROID)
    # ================= Windows 平台 =================
    set(SDL2_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2-2.0.20/lib/${ARCH_DIR}")

    target_link_libraries(sdl2_libs INTERFACE
        ${SDL2_LIB_DIR}/SDL2.lib
        ${SDL2_LIB_DIR}/SDL2main.lib
    )
    
    # 如果将来扩展了 SDL2_image / SDL2_mixer，在这里取消注释并追加：
    # set(SDL2_IMAGE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2_image-2.0.5/lib/${ARCH_DIR}")
    # target_include_directories(sdl2_libs INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2_image-2.0.5/include")
    # target_link_libraries(sdl2_libs INTERFACE ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib)

else()
    # ================= Android / Linux 平台 =================
    # C4droid 下，使用链接器参数链接系统里的 SDL2 动态库/静态库
    # 注意：如果你在 C4droid 报错找不到 -lSDL2，可能是因为它的库不在默认搜索路径
    # 可以在顶层 CMakeLists 里加 link_directories("/data/user/0/com.n0n3m4.droidc/files/...")
    target_link_libraries(sdl2_libs INTERFACE 
        -Wl,--whole-archive -lSDL2 -Wl,--no-whole-archive
    )
endif()

# ============================================================
# 3. 添加其他子模块
# ============================================================
add_subdirectory(imgui-1.89.9)
add_subdirectory(tinyxml2)

# ============================================================
# 4. IDE 目录分类 (仅在 VS / Xcode 下生效)
# ============================================================
if(MSVC OR XCODE)
    set_target_properties(sdl2_libs imgui tinyxml2 PROPERTIES FOLDER "External")
endif()