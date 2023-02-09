cmake_minimum_required(VERSION 3.16)
project(spacemouse)

if(NOT DEFINED ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "Missing required environment variable CCP_EVE_PERFORCE_BRANCH_PATH")
elseif(NOT IS_ABSOLUTE $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be an absolute path")
elseif(NOT EXISTS $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH is pointing at a non-existing location $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
elseif(NOT IS_DIRECTORY $ENV{CCP_EVE_PERFORCE_BRANCH_PATH})
    message(FATAL_ERROR "CCP_EVE_PERFORCE_BRANCH_PATH needs to be a directory")
else()
    message(STATUS "Using CCP_EVE_PERFORCE_BRANCH_PATH located at $ENV{CCP_EVE_PERFORCE_BRANCH_PATH}")
endif()

file(TO_CMAKE_PATH "$ENV{CCP_EVE_PERFORCE_BRANCH_PATH}" BRANCH_ROOT_DIR)
list(INSERT CMAKE_MODULE_PATH 0 ${BRANCH_ROOT_DIR}/cmake)
include(CcpGlobalSettings)
include(CcpPackageConfigHelpers)

find_package(Python REQUIRED NO_CMAKE_PATH)
find_package(3dwaresdk REQUIRED)


set(SRC_FILES
    SpaceMouse.cpp
    stdafx.cpp
)

# Add Platform specific code
if(WIN32)
    list(APPEND SRC_FILES SpaceMouse_Windows.cpp)
elseif(APPLE)
    list(APPEND SRC_FILES SpaceMouse_macOS.mm)
else()
	message(FATAL_ERROR "Spacemouse does not support platform specified.")
endif()

set(PUBLIC_HEADER_FILES "")

ccp_add_library(spacemouse SHARED ${SRC_FILES})

target_link_libraries(spacemouse PRIVATE Python 3dwaresdk)

get_target_property(_SOURCES spacemouse SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)

target_precompile_headers(spacemouse PRIVATE stdafx.h)
if(APPLE)
    enable_language(OBJCXX)
endif()

set_target_properties(spacemouse PROPERTIES OUTPUT_NAME "_spacemouse")

# Provide an install target iff this is the top level project only
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
    install(
        TARGETS spacemouse
        EXPORT spacemouseTargets
        CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
        ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
    )
    # Install rule for available public headers
    install(FILES ${PUBLIC_HEADER_FILES} DESTINATION include)

    message(STATUS "lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/")

    configure_ccp_vendor_config_file(
        TARGET spacemouse
        DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/spacemouseConfig.cmake"
        DEPENDENCIES ""
    )
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/spacemouseConfig.cmake DESTINATION .)
endif()