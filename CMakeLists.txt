project(spacemouse VERSION 1.0.0)

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
