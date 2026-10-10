cmake_minimum_required(VERSION 3.17)

# ---------- SDL2 预编译库（INTERFACE） ----------
add_library(sdl2_libs INTERFACE)

set(SDL2_LIB_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2-2.0.20/lib/${ARCH_DIR}")
#set(SDL2_IMAGE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2_image-2.0.5/lib/${ARCH_DIR}")

target_include_directories(sdl2_libs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2-2.0.20/include
  #  ${CMAKE_CURRENT_SOURCE_DIR}/SDL2/SDL2_image-2.0.5/include
)

target_link_libraries(sdl2_libs INTERFACE
    ${SDL2_LIB_DIR}/SDL2.lib
    ${SDL2_LIB_DIR}/SDL2main.lib
  #  ${SDL2_IMAGE_LIB_DIR}/SDL2_image.lib
)

# ---------- 各子模块 ----------
add_subdirectory(imgui-1.89.9)
add_subdirectory(tinyxml2)
add_subdirectory(ImGuiFileDialog)

# ★ FOLDER 分组
set_target_properties(sdl2_libs imgui tinyxml2 ImGuiFileDialog
    PROPERTIES FOLDER "External")