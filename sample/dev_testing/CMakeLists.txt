function(set_dev_sample_defaults target_name)
    target_include_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/include)
endfunction()

# add_executable(test_logger test_logger.cpp)
# target_link_libraries(test_logger PRIVATE evrgb_combo)
# set_dev_sample_defaults(test_logger)

add_executable(test_state_machine test_state_machine.cpp)
target_link_libraries(test_state_machine PRIVATE evrgb_combo ${MVS_LIBRARIES})
set_dev_sample_defaults(test_state_machine)
target_include_directories(test_state_machine PRIVATE ${MVS_INCLUDE_DIR})

add_executable(test_combo_dvs_callback test_combo_dvs_callback.cpp)
target_link_libraries(test_combo_dvs_callback PRIVATE evrgb_combo ${MVS_LIBRARIES})
set_dev_sample_defaults(test_combo_dvs_callback)
target_include_directories(test_combo_dvs_callback PRIVATE ${MVS_INCLUDE_DIR})

add_executable(test_synced_callback test_synced_callback.cpp)
target_link_libraries(test_synced_callback PRIVATE evrgb_combo ${MVS_LIBRARIES})
set_dev_sample_defaults(test_synced_callback)
target_include_directories(test_synced_callback PRIVATE ${MVS_INCLUDE_DIR})

add_executable(test_interface_compatibility test_interface_compatibility.cpp)
target_link_libraries(test_interface_compatibility PRIVATE evrgb_combo ${MVS_LIBRARIES})
set_dev_sample_defaults(test_interface_compatibility)
target_include_directories(test_interface_compatibility PRIVATE ${MVS_INCLUDE_DIR})

add_executable(test_device_model_name test_device_model_name.cpp)
target_link_libraries(test_device_model_name PRIVATE evrgb_combo ${MVS_LIBRARIES})
set_dev_sample_defaults(test_device_model_name)
target_include_directories(test_device_model_name PRIVATE ${MVS_INCLUDE_DIR})
