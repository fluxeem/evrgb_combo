# - Find MVS (Hikrobot Machine Vision Software) SDK
# 查找海康 MVS SDK (Machine Vision Software) 头文件和库文件。
#
# 调用后将定义以下变量：
#   MVS_FOUND           - 是否找到 MVS 库
#   MVS_INCLUDE_DIRS    - MVS 头文件目录
#   MVS_LIBRARIES       - MVS 链接库
#
# 并提供 CMake 目标：
#   MVS::MVS
#
# 示例：
#   find_package(MVS REQUIRED)
#   target_link_libraries(myapp PRIVATE MVS::MVS)

# 默认搜索路径（可在外部通过 CACHE 覆盖）
if(NOT DEFINED ENV{MVCAM_COMMON_RUNENV})
    message(FATAL_ERROR "Environment variable MVCAM_COMMON_RUNENV not found. Please install Hikrobot MVS SDK first.")
else()
    message(STATUS "Found MVCAM_COMMON_RUNENV: $ENV{MVCAM_COMMON_RUNENV}")
endif()

if(WIN32)
    set(MVS_LIB_SEARCH_PATHS
        $ENV{MVCAM_COMMON_RUNENV}/Libraries/win64
        $ENV{MVCAM_COMMON_RUNENV}/Libraries/win32
        CACHE PATH "Path(s) to MVS library directory"
    )

    set(MVS_INCLUDE_SEARCH_PATHS
        $ENV{MVCAM_COMMON_RUNENV}/Includes
        CACHE PATH "Path(s) to MVS include directory"
    )
else()
    set(MVS_LIB_SEARCH_PATHS
        "/opt/MVS/lib/64"
        "/opt/MVS/lib/32"
        CACHE PATH "Path(s) to MVS library directory"
    )

    set(MVS_INCLUDE_SEARCH_PATHS
        "/opt/MVS/include"
        CACHE PATH "Path(s) to MVS include directory"
    )
endif()

# 查找头文件目录
find_path(MVS_INCLUDE_DIR
    NAMES
        "MvCameraControl.h"
    PATHS
        ${MVS_INCLUDE_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

# 查找库文件
find_library(MVS_LIBRARY
    NAMES
        MvCameraControl
        libMvCameraControl
    PATHS
        ${MVS_LIB_SEARCH_PATHS}
    NO_DEFAULT_PATH
)

# 处理查找结果
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MVS
    REQUIRED_VARS MVS_LIBRARY MVS_INCLUDE_DIR
    FAIL_MESSAGE "Could not find Hikrobot MVS SDK. Please set MVS_LIB_SEARCH_PATHS and MVS_INCLUDE_SEARCH_PATHS."
)

if(MVS_FOUND)
    set(MVS_INCLUDE_DIRS ${MVS_INCLUDE_DIR})
    set(MVS_LIBRARIES ${MVS_LIBRARY})

    # 创建目标以方便链接
    if(NOT TARGET MVS::MVS)
        add_library(MVS::MVS UNKNOWN IMPORTED)
        set_target_properties(MVS::MVS PROPERTIES
            IMPORTED_LOCATION "${MVS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MVS_INCLUDE_DIR}"
        )
    endif()

    message(STATUS "Found MVS include: ${MVS_INCLUDE_DIR}")
    message(STATUS "Found MVS library: ${MVS_LIBRARY}")
endif()
