set(FILE_DIALOG_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialogConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ImGuiFileDialog.cpp
)

add_library(ImGuiFileDialog STATIC ${FILE_DIALOG_FILES})

target_include_directories(ImGuiFileDialog PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ImGuiFileDialog PUBLIC imgui)

target_compile_features(ImGuiFileDialog PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(ImGuiFileDialog PRIVATE /utf-8)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             PREFIX "Source Files"
             FILES ${FILE_DIALOG_FILES})