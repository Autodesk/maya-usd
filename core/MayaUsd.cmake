message("MayaUsd.cmake")

# Convert some vars to forward slashes (if required) to fix invalid escape
# sequence parsing errors.
file(TO_CMAKE_PATH ${CMAKE_MAKE_PROGRAM} CMAKE_MAKE_PROGRAM)
file(TO_CMAKE_PATH ${MAYA_LOCATION} MAYA_LOCATION)
file(TO_CMAKE_PATH ${MAYAUSD_BUILD_ROOT} MAYAUSD_BUILD_ROOT)

message("CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}")
message("CMAKE_MAKE_PROGRAM = ${CMAKE_MAKE_PROGRAM}")
message("MAYA_LOCATION = ${MAYA_LOCATION}")
message("MAYAUSD_BUILD_ROOT = ${MAYAUSD_BUILD_ROOT}")
message("PXR_USD_LOCATION = ${PXR_USD_LOCATION}")
message("USD_CONFIG_FILE = ${USD_CONFIG_FILE}")

# Helper macro to add the needed defines to a list so that we can easily pass
# them to the external project. That external project is executed in a separate
# cmake process meaning it doesn't automatically inherit all these variables.
macro(add_mayausd_define var)
    if (DEFINED ${var})
        list(APPEND MAYAUSD_DEFINES -D${var}=${${var}})
    endif()
endmacro(add_mayausd_define)

# 
add_mayausd_define(MAYA_LOCATION)

set(MAYAUSD_INSTALL_DIR "${MAYAUSD_BUILD_ROOT}/mayausd-install/mayaUsd-${MAYAUSD_MAJOR_VERSION}-${MAYAUSD_MINOR_VERSION}-${MAYAUSD_PATCH_LEVEL}")

# Configure the file that contains our external project command.
if (NOT CMAKE_VERSION VERSION_LESS "3.1.3")
    set(MAYAUSD_BUILD_ALWAYS "BUILD_ALWAYS ON")
endif()
configure_file(core/CMakeLists_MayaUsd.txt.in ${MAYAUSD_BUILD_ROOT}/mayausd-config/CMakeLists.txt)

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
execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${MAYAUSD_BUILD_ROOT}/mayausd-config )
if(result)
    message(FATAL_ERROR "Build step for MayaUSD failed: ${result}")
endif()
message(STATUS "========== ...  MayaUSD installed. ==========")


set(MAYAUSD_INCLUDE_ROOT ${MAYAUSD_INSTALL_DIR})
set(MAYAUSD_LIB_ROOT ${MAYAUSD_INSTALL_DIR})

set(INSTALL_DIR_SUFFIX MayaUSD)

# Install the entire MayaUsd folder into the same folder as ${CMAKE_INSTALL_PREFIX}.
# Note: the trailing slash is intentional so we won't end up with a 'mayaUsd' sub-folder.
#       we also exlude the *.lib files.
install(
    DIRECTORY ${MAYAUSD_INSTALL_DIR}/
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${INSTALL_DIR_SUFFIX}
    PATTERN *.lib EXCLUDE
    PATTERN include EXCLUDE
)
