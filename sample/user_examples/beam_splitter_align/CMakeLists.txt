cmake_minimum_required(VERSION 3.22)
project(beam_splitter_align LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET evrgb_combo)
    find_package(evrgb_combo CONFIG REQUIRED)
endif()

find_package(MVS REQUIRED)

add_executable(beam_splitter_align beam_splitter_align.cpp)

target_link_libraries(beam_splitter_align PRIVATE
    evrgb_combo
    ${MVS_LIBRARIES}
)

target_include_directories(beam_splitter_align PRIVATE
    ${MVS_INCLUDE_DIR}
)