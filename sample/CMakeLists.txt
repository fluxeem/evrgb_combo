option(BUILD_DEV_SAMPLES "Build internal development samples" OFF)

add_subdirectory(user_examples)

if(BUILD_DEV_SAMPLES)
    add_subdirectory(dev_testing)
endif()

