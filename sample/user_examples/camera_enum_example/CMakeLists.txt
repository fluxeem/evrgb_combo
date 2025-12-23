cmake_minimum_required(VERSION 3.22)
project(camera_enum_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET evrgb_combo)
    find_package(evrgb_combo CONFIG REQUIRED)
endif()

find_package(MVS REQUIRED)

add_executable(camera_enum_example camera_enum_example.cpp)

target_link_libraries(camera_enum_example PRIVATE
    evrgb_combo
    ${MVS_LIBRARIES}
)

target_include_directories(camera_enum_example PRIVATE
    ${MVS_INCLUDE_DIR}
)
