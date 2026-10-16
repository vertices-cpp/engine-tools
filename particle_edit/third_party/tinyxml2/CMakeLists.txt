set(TINYXML2_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tinyxml2.cpp
)

add_library(tinyxml2 STATIC ${TINYXML2_FILES})

target_include_directories(tinyxml2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
             PREFIX "Source Files"
             FILES ${TINYXML2_FILES})