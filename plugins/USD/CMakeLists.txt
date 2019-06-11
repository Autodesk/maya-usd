message("*************** In PIXAR plugin  ***************")

# Explicitly set these variables for Linux
if(NOT WIN32)
    set(OPENEXR_LOCATION ${PXR_USD_LOCATION})
    set(GLEW_LOCATION ${PXR_USD_LOCATION})
    set(OPENSUBDIV_ROOT_DIR ${PXR_USD_LOCATION})
    set(TBB_ROOT_DIR ${PXR_USD_LOCATION})
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${PROJECT_SOURCE_DIR}/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/defaults
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macros)

if (PXR_BUILD_MAYA_PLUGIN)
    find_package(Maya REQUIRED)
endif()

find_package(USD REQUIRED)
include(${USD_CONFIG_FILE})

# find_package(MayaCoreUSD REQUIRED)

include(Options)
include(Public)

# This has to be defined after Packages is included, because it relies on the
# discovered path to the python executable.
set(PXR_PYTHON_SHEBANG "${PYTHON_EXECUTABLE}" 
    CACHE 
    STRING
    "Replacement path for Python #! line."
)

# CXXDefaults will set a variety of variables for the project.
# Consume them here. This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${_PXR_CXX_FLAGS} ${CMAKE_CXX_FLAGS}")

pxr_toplevel_prologue()

if (${PXR_BUILD_MAYA_PLUGIN})
    add_subdirectory(third_party/maya)
endif()

pxr_toplevel_epilogue()
