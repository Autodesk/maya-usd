# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PROJECT_NAME}
    PRIVATE
        moduleDeps.cpp
)

# Python extension library for mayaUsd library bindings. Pixar's TF_WRAP_MODULE
# macro expects the library name to be prefixed with _, but adds an underscore
# to the package name when creating the Python extension module initialization
# function (see Pixar's pyModule.h).
if(IS_WINDOWS AND MAYAUSD_DEFINE_BOOST_DEBUG_PYTHON_FLAG)
    # On Windows when compiling with debug python the library must be named with _d.
    set(PYTHON_TARGET_NAME _${PROJECT_NAME}_d)
else()
    set(PYTHON_TARGET_NAME _${PROJECT_NAME})
endif()

add_library(${PYTHON_TARGET_NAME} SHARED)

# -----------------------------------------------------------------------------
# sources
# -----------------------------------------------------------------------------
target_sources(${PYTHON_TARGET_NAME} 
    PRIVATE
        module.cpp
        wrapAdaptor.cpp
        wrapBlockSceneModificationContext.cpp
        wrapColorSpace.cpp
        wrapConverter.cpp
        wrapDiagnosticDelegate.cpp
        wrapEditRouter.cpp
        wrapMeshWriteUtils.cpp
        wrapQuery.cpp
        wrapReadUtil.cpp
        wrapRoundTripUtil.cpp
        wrapStageCache.cpp
        wrapTokens.cpp
        wrapUsdUndoManager.cpp
        wrapUserTaggedAttribute.cpp
        wrapWriteUtil.cpp
        wrapXformStack.cpp

        wrapOpenMaya.cpp
        wrapPrimReader.cpp
        wrapPrimWriter.cpp
        wrapExportChaser.cpp
        wrapImportChaser.cpp
        wrapJobContextRegistry.cpp
        wrapSchemaApiAdaptor.cpp
        wrapShadingUtil.cpp
        wrapShadingMode.cpp
)

# Edit as Maya requires UFE path mapping.
if(CMAKE_UFE_V3_FEATURES_AVAILABLE)
    target_sources(${PYTHON_TARGET_NAME}
        PRIVATE
            wrapPrimUpdater.cpp
            wrapPrimUpdaterManager.cpp
    )
endif()

# -----------------------------------------------------------------------------
# compiler configuration
# -----------------------------------------------------------------------------
target_compile_definitions(${PYTHON_TARGET_NAME}
    PRIVATE
        $<$<BOOL:${IS_MACOSX}>:OSMac_>
        MFB_PACKAGE_NAME=${PROJECT_NAME}
        MFB_ALT_PACKAGE_NAME=${PROJECT_NAME}
        MFB_PACKAGE_MODULE=${PROJECT_NAME}
)

mayaUsd_compile_config(${PYTHON_TARGET_NAME})

# -----------------------------------------------------------------------------
# link libraries
# -----------------------------------------------------------------------------
target_link_libraries(${PYTHON_TARGET_NAME}
    PRIVATE
        ${PROJECT_NAME}
)

# -----------------------------------------------------------------------------
# properties
# -----------------------------------------------------------------------------
set_python_module_property(${PYTHON_TARGET_NAME})

# -----------------------------------------------------------------------------
# run-time search paths
# -----------------------------------------------------------------------------
if(IS_MACOSX OR IS_LINUX)
    mayaUsd_init_rpath(rpath "lib")
    mayaUsd_add_rpath(rpath "../../..")
    if(IS_MACOSX AND DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        mayaUsd_add_rpath(rpath "../../../../../../Maya.app/Contents/MacOS")
        mayaUsd_add_rpath(rpath "../../../../${MAYAUSD_TO_USD_RELATIVE_PATH}/lib")
    endif()
    mayaUsd_install_rpath(rpath ${PYTHON_TARGET_NAME})
endif()

# -----------------------------------------------------------------------------
# install
# -----------------------------------------------------------------------------
# We will use the following Python package structure:
#
# mayaUsd: top-level package
# mayaUsd.lib: Python wrappers for mayaUsd library functionality.
#
# We re-map the Python library namespace to mayaUsd.lib through a simple
# __init__.py.
set(PYLIB_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/lib/python/${PROJECT_NAME}/lib)

install(TARGETS ${PYTHON_TARGET_NAME}
    LIBRARY
    DESTINATION ${PYLIB_INSTALL_PREFIX}
    ARCHIVE
    DESTINATION ${PYLIB_INSTALL_PREFIX}
    RUNTIME
    DESTINATION ${PYLIB_INSTALL_PREFIX}
)

if(IS_WINDOWS)
    install(FILES $<TARGET_PDB_FILE:${PYTHON_TARGET_NAME}> 
            DESTINATION ${PYLIB_INSTALL_PREFIX} OPTIONAL)
endif()

install(FILES __init__.py DESTINATION ${PYLIB_INSTALL_PREFIX})
