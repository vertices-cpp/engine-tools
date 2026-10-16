# 手动列（排除 imgui_demo.cpp）
set(IMGUI_HEADERS
    imconfig.h
    imgui.h
    imgui_internal.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    backends/imgui_impl_sdl2.h
    backends/imgui_impl_sdlrenderer2.h
)

set(IMGUI_SOURCES
    imgui.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_sdl2.cpp
    backends/imgui_impl_sdlrenderer2.cpp
)

# 拼绝对路径
set(IMGUI_ALL "")
foreach(f ${IMGUI_HEADERS} ${IMGUI_SOURCES})
    list(APPEND IMGUI_ALL "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
endforeach()

add_library(imgui STATIC ${IMGUI_ALL})

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC sdl2_libs)

# ★ 全部进 Source Files
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             PREFIX "Source Files"
             FILES ${IMGUI_ALL})