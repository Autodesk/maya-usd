# https://vicrucann.github.io/tutorials/quick-cmake-doxygen/

find_package(Doxygen REQUIRED)

set(DOXYGEN_IN "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in")
set(DOXYGEN_OUT "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
add_custom_target(docs_doxygen ALL
                  DEPENDS
                    ${CMAKE_CURRENT_SOURCE_DIR}/../README.md
                    ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
                    ${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTING.md
                    hdmaya
                    mtoh
                  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                  COMMENT "Generating API documentation with Doxygen"
                  VERBATIM)

install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs/html/"
        DESTINATION docs)
