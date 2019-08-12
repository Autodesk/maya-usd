include(ProcessorCount)

# Convert some vars to forward slashes (if required) to fix invalid escape
# sequence parsing errors.
file(TO_CMAKE_PATH ${CMAKE_MAKE_PROGRAM} CMAKE_MAKE_PROGRAM)
file(TO_CMAKE_PATH ${MAYA_LOCATION} MAYA_LOCATION)
file(TO_CMAKE_PATH ${PXR_USD_LOCATION} PXR_USD_LOCATION)
file(TO_CMAKE_PATH ${MAYAUSD_BUILD_ROOT} MAYAUSD_BUILD_ROOT)

# MAYA_DEVKIT_LOCATION is required for UFE-based features. Use the
# "--devkit-location" flag to specify the directory of Maya devkit.
if(NOT "${MAYA_DEVKIT_LOCATION}" STREQUAL "")
    file(TO_CMAKE_PATH ${MAYA_DEVKIT_LOCATION} MAYA_DEVKIT_LOCATION)
endif()

message("CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
message("CMAKE_MAKE_PROGRAM = ${CMAKE_MAKE_PROGRAM}")
message("MAYAUSD_BUILD_ROOT = ${MAYAUSD_BUILD_ROOT}")

# Helper macro to add the needed defines to a list so that we can easily pass
# them to the external project. That external project is executed in a separate
# cmake process meaning it doesn't automatically inherit all these variables.
macro(add_mayausd_define var)
    if (DEFINED ${var})
        list(APPEND MAYAUSD_DEFINES -D${var}=${${var}})
        message("${var} = ${${var}}")
    endif()
endmacro(add_mayausd_define)

# Add top level variabels here
add_mayausd_define(CMAKE_WANT_UFE_BUILD)
add_mayausd_define(MAYAUSD_VERSION)
add_mayausd_define(MAYAUSD_MAJOR_VERSION)
add_mayausd_define(MAYAUSD_MINOR_VERSION)
add_mayausd_define(MAYAUSD_PATCH_LEVEL)
add_mayausd_define(MAYA_LOCATION)
add_mayausd_define(MAYA_DEVKIT_LOCATION)
add_mayausd_define(PXR_USD_LOCATION)
add_mayausd_define(CMAKE_CXX_FLAGS)
add_mayausd_define(PYTHON_INCLUDE_DIR)
add_mayausd_define(PYTHON_LIBRARIES)
add_mayausd_define(PYTHON_EXECUTABLE)
add_mayausd_define(UFE_INCLUDE_ROOT)
add_mayausd_define(UFE_LIB_ROOT)

set(MAYAUSD_INSTALL_DIR "${MAYAUSD_BUILD_ROOT}/mayausd-install/mayaUsd-${MAYAUSD_MAJOR_VERSION}-${MAYAUSD_MINOR_VERSION}-${MAYAUSD_PATCH_LEVEL}")

# Configure the file that contains our external project command.
if (NOT CMAKE_VERSION VERSION_LESS "3.1.3")
    set(MAYAUSD_BUILD_ALWAYS "BUILD_ALWAYS ON")
endif()
configure_file(lib/CMakeLists_MayaUsd.txt.in ${MAYAUSD_BUILD_ROOT}/mayausd-config/CMakeLists.txt)

# Execute the MayaUSD build right now.
message(STATUS "========== Building MayaUSD_v${MAYAUSD_VERSION} ... ==========")
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${MAYAUSD_BUILD_ROOT}/mayausd-config )
if(result)
    message(FATAL_ERROR "CMake step for MayaUSD failed: ${result}")
endif()

# If we are passed in extra build flags, we add them here.
# Note: need to figure out how to pass this variable which contains the string:
#        /nr:false /m /p:Configuration=RelWithDebInfo;Architecture=x64
#       The problem is the "/ : ;" are being interpreted and causing problems.
#       How to pass this var just as a plain string?
# execute_process(COMMAND ${CMAKE_COMMAND} --build . -- ${MAYAUSD_CMAKE_BUILD_FLAGS}
#

ProcessorCount(N)
# TODO: execute_process doesn't like it's argument to be passed as cmake variables?
if( CMAKE_GENERATOR MATCHES "Make" OR CMAKE_GENERATOR MATCHES "Unix Makefiles" OR CMAKE_GENERATOR MATCHES "Ninja")
    execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} -j ${N}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${MAYAUSD_BUILD_ROOT}/mayausd-config )
    if(result)
        message(FATAL_ERROR "Build step for MayaUSD failed: ${result}")
    endif()
else()
    execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} -- /M:${N}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${MAYAUSD_BUILD_ROOT}/mayausd-config )
    if(result)
        message(FATAL_ERROR "Build step for MayaUSD failed: ${result}")
    endif()
endif()

message(STATUS "========== ...  MayaUSD installed. ==========")

set(MAYAUSD_INCLUDE_ROOT ${MAYAUSD_INSTALL_DIR})
set(MAYAUSD_LIB_ROOT ${MAYAUSD_INSTALL_DIR})

#==============================================================================
# Install
#==============================================================================
set(INSTALL_MAYAUSD_INCLUDE_DIR ${MAYAUSD_INSTALL_DIR}/include)
set(INSTALL_MAYAUSD_LIB_DIR ${MAYAUSD_INSTALL_DIR}/lib)

# install header
install(
    DIRECTORY ${INSTALL_MAYAUSD_INCLUDE_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

# install lib
install(
    DIRECTORY ${INSTALL_MAYAUSD_LIB_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    PATTERN *.lib EXCLUDE
)
