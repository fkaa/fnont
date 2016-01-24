cmake_minimum_required(VERSION 2.6)
project(fnont C)

find_package(Freetype REQUIRED)

include_directories(
    "${FREETYPE_INCLUDE_DIRS}"
    "${fnont_SOURCE_DIR}/include"
)

set(HEADER_FILES
    include/fnont.h
)

set(SOURCE_FILES
    src/fnont.c
)

add_library(fnont ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(fnont
    ${FREETYPE_LIBRARIES}
)

set(DEMO_FILES
    demo/main.c
)

add_executable(fnont_demo ${DEMO_FILES})

target_link_libraries(fnont_demo
    fnont
    ${FREETYPE_LIBRARIES}
)

