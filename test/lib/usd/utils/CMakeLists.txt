set(TARGET_NAME DiffCore)

add_executable(${TARGET_NAME})

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${TARGET_NAME}
    PRIVATE
        main.cpp
        test_DiffCore.cpp
)

# -----------------------------------------------------------------------------
# include directories
# -----------------------------------------------------------------------------
target_include_directories(${TARGET_NAME} 
    PRIVATE 
        ${GTEST_INCLUDE_DIRS}
        ${USD_INCLUDE_DIR}
)

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${TARGET_NAME}
    PRIVATE 
        ${GTEST_LIBRARIES}
        mayaUsdUtils
)

# -----------------------------------------------------------------------------
# test
# -----------------------------------------------------------------------------
set(path
    "${CMAKE_INSTALL_PREFIX}/lib"
    "${CMAKE_INSTALL_PREFIX}/lib/usd"
    "$ENV{PATH}"
)

if(IS_WINDOWS)
    list(APPEND path "${CMAKE_INSTALL_PREFIX}/lib/gtest")
endif()

separate_argument_list(path)

add_test(NAME ${TARGET_NAME} 
         COMMAND $<TARGET_FILE:${TARGET_NAME}>
)

set_property(TEST ${TARGET_NAME} APPEND PROPERTY ENVIRONMENT 
    "PATH=${path}"
)
