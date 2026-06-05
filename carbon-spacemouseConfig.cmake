# Copyright © 2026 CCP ehf.

include(CMakeFindDependencyMacro)
include("${CMAKE_CURRENT_LIST_DIR}/carbon-spacemouse.cmake")


find_dependency(Python3 COMPONENTS Development REQUIRED)
find_dependency(3dxwaresdk REQUIRED)
