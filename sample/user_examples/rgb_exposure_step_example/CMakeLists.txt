cmake_minimum_required(VERSION 3.22)
project(rgb_exposure_step_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET evrgb_combo)
    find_package(evrgb_combo CONFIG REQUIRED)
endif()

find_package(MVS REQUIRED)
find_package(OpenCV REQUIRED)

add_executable(rgb_exposure_step_example rgb_exposure_step_example.cpp)

target_link_libraries(rgb_exposure_step_example PRIVATE
    evrgb_combo
    ${MVS_LIBRARIES}
    ${OpenCV_LIBS}
)

target_include_directories(rgb_exposure_step_example PRIVATE
    ${MVS_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)
