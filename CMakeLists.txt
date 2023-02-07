cmake_minimum_required(VERSION 3.16)
project(spacemouse VERSION 1.0.0)

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

find_package(Python REQUIRED NO_CMAKE_PATH)
find_package(3dwaresdk REQUIRED)

add_library(spacemouse SHARED)
target_link_libraries(spacemouse PRIVATE Python 3dwaresdk)

target_sources(spacemouse PRIVATE
    SpaceMouse.cpp
    stdafx.cpp
    stdafx.h
)

# Add Platform specific code
if(WIN32)
	target_sources(spacemouse PRIVATE 
        SpaceMouse_Windows.cpp
	)
elseif(APPLE)
	target_sources(spacemouse PRIVATE 
        SpaceMouse_macOS.mm
	)
else()
	message(FATAL_ERROR "Spacemouse does not support platform specified.")
endif()


get_target_property(_SOURCES spacemouse SOURCES)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Sources"
        FILES ${_SOURCES}
)


target_precompile_headers(spacemouse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/stdafx.h)
if(APPLE)
    enable_language(OBJCXX)
    set_target_properties(
        spacemouse
        PROPERTIES 
            PREFIX ""
            SUFFIX ".so"
    )
endif()

set_target_properties(spacemouse PROPERTIES OUTPUT_NAME "_spacemouse")
