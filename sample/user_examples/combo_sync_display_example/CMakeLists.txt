cmake_minimum_required(VERSION 3.22)
project(combo_sync_display_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET evrgb_combo)
    find_package(evrgb_combo CONFIG REQUIRED)
endif()

find_package(MVS REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(combo_sync_display_example combo_sync_display_example.cpp)

target_link_libraries(combo_sync_display_example PRIVATE
    evrgb_combo
    ${MVS_LIBRARIES}
    ${OpenCV_LIBS}
)

target_include_directories(combo_sync_display_example PRIVATE
    ${MVS_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
