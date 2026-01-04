find_package(MVS REQUIRED)
find_package(DvsenseDriver REQUIRED)

add_compile_definitions(EVRGB_EXPORTS)

add_library(evrgb_combo SHARED
    utils/calib_info.cpp
    utils/event_visualizer.cpp
    utils/evrgb_logger.cpp
    camera/i_camera.cpp
    camera/rgb_camera.cpp
    camera/dvs_camera.cpp
    core/combo_core.cpp
    core/combo_threads.cpp
    sync/event_vector_pool.cpp
    sync/trigger_buffer.cpp
    recording/synced_data_recorder.cpp
    recording/recorded_sync_reader.cpp
    ../include/core/version.h
)

# Link libraries.
target_link_libraries(evrgb_combo
    PUBLIC
        opencv_core opencv_imgproc opencv_highgui   
        DvsenseDriver::Driver 
    PRIVATE
        loguru::loguru
        nlohmann_json::nlohmann_json
        ${MVS_LIBRARIES}
)

# Use PUBLIC so that the include directories are available to consumers
target_include_directories(evrgb_combo 
PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${OpenCV_INCLUDE_DIRS}>
PRIVATE
    $<BUILD_INTERFACE:${MVS_INCLUDE_DIR}>
)

# Install the library
install(TARGETS evrgb_combo
        EXPORT evrgb_combo-targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
)

# Install the export targets
install(EXPORT evrgb_combo-targets
        FILE evrgb_combo-targets.cmake
        DESTINATION lib/cmake/evrgb_combo
)
