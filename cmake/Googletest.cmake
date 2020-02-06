macro(fetch_googletest)

    if (NOT GTEST_FOUND)
        # First see if we can find a gtest that was downloaded and built.
        if (NOT GOOGLETEST_BUILD_ROOT)
            set(GOOGLETEST_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR})
        endif()
        if (NOT GTEST_ROOT)
            set(GTEST_ROOT "${GOOGLETEST_BUILD_ROOT}/googletest-install")
        endif()
        find_package(GTest QUIET)
        # At this point GTEST_FOUND is set to True in Release but False in Debug.
    endif()

    if (NOT GTEST_FOUND)
        #======================================================================
        # Download and unpack googletest at configure time.  Adapted from
        #
        # https://github.com/abseil/googletest/blob/master/googletest/README.md
        #
        # PPT, 22-Nov-2018.

        # Immediately convert CMAKE_MAKE_PROGRAM to forward slashes (if required).
        # Attempting to do so in execute_process fails with string invalid escape
        # sequence parsing errors.  PPT, 22-Nov-2018.
        file(TO_CMAKE_PATH ${CMAKE_MAKE_PROGRAM} CMAKE_MAKE_PROGRAM)
        if (GOOGLETEST_SRC_DIR)
            configure_file(cmake/CMakeLists_googletest_src.txt.in ${GOOGLETEST_BUILD_ROOT}/googletest-config/CMakeLists.txt)
        else()
            configure_file(cmake/CMakeLists_googletest_download.txt.in ${GOOGLETEST_BUILD_ROOT}/googletest-config/CMakeLists.txt)
        endif()

        message(STATUS "========== Installing GoogleTest... ==========")
           execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${GOOGLETEST_BUILD_ROOT}/googletest-config )
        if(result)
            message(FATAL_ERROR "CMake step for googletest failed: ${result}")
        endif()

        execute_process(COMMAND "${CMAKE_COMMAND}" --build . --config ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${GOOGLETEST_BUILD_ROOT}/googletest-config )
        if(result)
            message(FATAL_ERROR "Build step for googletest failed: ${result}")
        endif()
        message(STATUS "========== ...  GoogleTest installed. ==========")

        set(GTEST_ROOT ${GOOGLETEST_BUILD_ROOT}/googletest-install CACHE path "GoogleTest installation root")
    endif()

    # FindGTest should get call after GTEST_ROOT is set
    find_package(GTest QUIET)

    # https://gitlab.kitware.com/cmake/cmake/issues/17799
    # FindGtest is buggy when dealing with Debug build.
    if (CMAKE_BUILD_TYPE MATCHES Debug AND GTEST_FOUND MATCHES FALSE)
        message("Setting GTest libraries with debug...")
        set(GTEST_INCLUDE_DIRS ${GTEST_ROOT}/include)
        set(GTEST_LIBRARY ${GTEST_LIBRARY_DEBUG})
        set(GTEST_LIBRARIES ${GTEST_LIBRARY})
        set(GTEST_MAIN_LIBRARY ${GTEST_MAIN_LIBRARY_DEBUG})
        set(GTEST_MAIN_LIBRARIES ${GTEST_MAIN_LIBRARY})
    endif()

    # On Windows shared libraries are installed in 'bin' instead of 'lib' directory.
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        set(GTEST_SHARED_LIB_NAME "gtest.dll")
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            set(GTEST_SHARED_LIB_NAME "gtestd.dll")
        endif()
        install(FILES "${GTEST_ROOT}/bin/${GTEST_SHARED_LIB_NAME}" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    else()
        install(FILES "${GTEST_LIBRARIES}" DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/gtest")
    endif()

endmacro()
