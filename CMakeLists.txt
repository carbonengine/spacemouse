cmake_minimum_required(VERSION 3.16)
project(spacemouse)

include(cmake/CcpBuildConfigurations.cmake)
include(cmake/CcpTargetConfigurations.cmake)
include(cmake/CcpPackageConfigHelpers.cmake)
include(cmake/CcpMonolithVendorConfig.cmake)
include(CMakePackageConfigHelpers)

find_package(Python3 COMPONENTS Development REQUIRED)
find_package(3dxwaresdk REQUIRED)


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

ccp_add_library(spacemouse SHARED ${SRC_FILES})

target_link_libraries(spacemouse PRIVATE Python3::Python 3dxwaresdk)

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

    option(INSTALL_TO_MONOLITH "Install this package to a perforce branch vendor folder - provided for backwards compatability" OFF)

    if(INSTALL_TO_MONOLITH)
        # Install rule to ensure that our runtime and linker files are in the expected, platform-specific folders
        install(
            TARGETS spacemouse
            EXPORT spacemouseTargets
            CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
            LIBRARY DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            ARCHIVE DESTINATION lib/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
            RUNTIME DESTINATION bin/${CCP_PLATFORM}/${CCP_ARCHITECTURE}/${CCP_TOOLSET}/
        )

        configure_ccp_vendor_config_file(
            TARGET spacemouse
            DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/spacemouseConfig.cmake"
            DEPENDENCIES ""
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/spacemouseConfig.cmake DESTINATION .)

         # Install python package
         install(DIRECTORY python/ DESTINATION python
            FILES_MATCHING
            PATTERN "*.py"
         )
 else()
        install(
                TARGETS spacemouse
                EXPORT spacemouseTargets
                CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
                LIBRARY DESTINATION lib/
                ARCHIVE DESTINATION lib/
                RUNTIME DESTINATION bin/
        )

        install(
                EXPORT spacemouseTargets
                DESTINATION share/carbon-spacemouse
                FILE carbon-spacemouse.cmake
        )
        install(
                FILES carbon-spacemouseConfig.cmake
                DESTINATION share/carbon-spacemouse
        )

 endif()
endif()