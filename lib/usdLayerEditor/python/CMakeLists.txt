# Python extension library for UsdLayerEditor library bindings. Pixar's TF_WRAP_MODULE
# macro expects the library name to be prefixed with _, but adds an underscore
# to the package name when creating the Python extension module initialization
# function (see Pixar's pyModule.h).

set(PYTHON_TARGET_NAME "_${PROJECT_NAME}")

add_library(${PYTHON_TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PYTHON_TARGET_NAME} 
    PRIVATE
        module.cpp
        wrapUsdLayerEditor.cpp
)

target_include_directories( ${PYTHON_TARGET_NAME} PUBLIC
	${CMAKE_SOURCE_DIR}/lib
)

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PYTHON_TARGET_NAME}
    PRIVATE
        MFB_PACKAGE_NAME=${PROJECT_NAME}
        MFB_ALT_PACKAGE_NAME=${PROJECT_NAME}
        MFB_PACKAGE_MODULE=${PROJECT_NAME}
)

if(MSVC)
	# Prevent the removal of unreferenced code. The MSVC compiler
	# makes mistakes and remove code that is actually in use, with the
	# PXR registration macros.
	target_compile_options(${PYTHON_TARGET_NAME} PRIVATE /Zc:inline-)
	# Parallel compile
 	add_definitions(/MP)
endif()

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PYTHON_TARGET_NAME}
    PRIVATE
        ${PROJECT_NAME}
)

set_target_properties(${PYTHON_TARGET_NAME}
	PROPERTIES
		PREFIX ""
		SUFFIX ".pyd"
)

set(PYLIB_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/python/${PROJECT_NAME})

install(TARGETS ${PYTHON_TARGET_NAME}
    LIBRARY
        DESTINATION ${PYLIB_INSTALL_PREFIX}
    ARCHIVE
        DESTINATION ${PYLIB_INSTALL_PREFIX}
    RUNTIME
        DESTINATION ${PYLIB_INSTALL_PREFIX}
)

install(FILES __init__.py DESTINATION ${PYLIB_INSTALL_PREFIX})

install(FILES $<TARGET_PDB_FILE:${PYTHON_TARGET_NAME}> 
		DESTINATION ${PYLIB_INSTALL_PREFIX} OPTIONAL)
