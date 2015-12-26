cmake_minimum_required(VERSION 2.6)
project(fnont C)

find_package(Freetype REQUIRED)

include_directories(
    "${FREETYPE_INCLUDE_DIRS}"
    "${fnont_SOURCE_DIR}/include"
)

set(SOURCE_FILES
    src/fnont.c
)

add_definitions(-fvisibility=hidden)


add_library(fnont
    ${SOURCE_FILES}
)

target_link_libraries(fnont
    ${FREETYPE_LIBRARIES}
)
