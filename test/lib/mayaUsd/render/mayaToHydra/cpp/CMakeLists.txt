# -----------------------------------------------------------------------------
# setup
# -----------------------------------------------------------------------------
find_package(GTest REQUIRED)

set(TARGET_NAME mayaHydraCppTests)
add_library(${TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        mayaHydraCppTestsCmd.cpp
        testCppFramework.cpp
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
mayaUsd_compile_config(${TARGET_NAME})

target_compile_definitions(${TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        $<$<BOOL:${IS_LINUX}>:LINUX>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        $<$<BOOL:${IS_WINDOWS}>:GTEST_LINKED_AS_SHARED_LIBRARY>
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PUBLIC
        ${MAYA_INCLUDE_DIRS}
        ${GTEST_INCLUDE_DIRS}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PUBLIC 
        ${GTEST_LIBRARIES}
        ${MAYA_LIBRARIES}
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
maya_set_plugin_properties(${TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX) 
    mayaUsd_init_rpath(rpath "lib/maya")
    mayaUsd_add_rpath(rpath "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    mayaUsd_install_rpath(rpath ${TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
install(TARGETS ${TARGET_NAME}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/maya)