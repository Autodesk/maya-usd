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
        set(FORCE_SHARED_CRT "")
        if(IS_WINDOWS)
            set(FORCE_SHARED_CRT -DFORCE_SHARED_CRT=OFF)
        endif()
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} . ${FORCE_SHARED_CRT}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${GOOGLETEST_BUILD_ROOT}/googletest-config )
        if(result)
            message(FATAL_ERROR "CMake step for googletest failed: ${result}")
        endif()

        execute_process(COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE}
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${GOOGLETEST_BUILD_ROOT}/googletest-config )
        if(result)
            message(FATAL_ERROR "Build step for googletest failed: ${result}")
        endif()
        message(STATUS "========== ...  GoogleTest installed. ==========")

        set(GTEST_ROOT ${GOOGLETEST_BUILD_ROOT}/googletest-install CACHE path "GoogleTest installation root")
    endif()

    # https://gitlab.kitware.com/cmake/cmake/issues/17799
    # FindGtest is buggy when dealing with Debug build. 
    if (CMAKE_BUILD_TYPE MATCHES Debug AND GTEST_FOUND MATCHES FALSE)
        message("Setting GTest libraries with debug...")

        if (GTEST_LIBRARY_DEBUG MATCHES GTEST_LIBRARY_DEBUG-NOTFOUND)
                set(gtest_library "")
                set(gtest_main_library "")
            if(WIN32)
                set(gtest_library lib/gtestd.lib)
                set(gtest_main_library lib/gtest_maind.lib)
            else()
                set(gtest_library lib64/libgtestd.a)
                set(gtest_main_library lib64/libgtest_maind.a)
            endif()
                set(GTEST_INCLUDE_DIRS ${GOOGLETEST_BUILD_ROOT}/googletest-install/include)
                set(GTEST_LIBRARY_DEBUG ${GOOGLETEST_BUILD_ROOT}/googletest-install/${gtest_library})
                set(GTEST_MAIN_LIBRARY_DEBUG ${GOOGLETEST_BUILD_ROOT}/googletest-install/${gtest_main_library})
        endif()

        set(GTEST_LIBRARY ${GTEST_LIBRARY_DEBUG})
        set(GTEST_LIBRARIES ${GTEST_LIBRARY})
        set(GTEST_MAIN_LIBRARY ${GTEST_MAIN_LIBRARY_DEBUG})
        set(GTEST_MAIN_LIBRARIES ${GTEST_MAIN_LIBRARY})
    endif()

endmacro()
