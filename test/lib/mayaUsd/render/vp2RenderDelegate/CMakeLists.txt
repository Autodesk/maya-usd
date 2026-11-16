# Unit test scripts.
set(TEST_SCRIPT_FILES_LAMBERT "")

list(APPEND TEST_SCRIPT_FILES_LAMBERT
    testVP2RenderDelegateDisplayColors.py
	testVP2RenderDelegateGeomSubset.py
    testVP2RenderDelegatePayloadBBox.py
    testVP2RenderDelegatePointInstanceOrientation.py
    testVP2RenderDelegateTextureLoading.py
    testVP2RenderDelegateDuplicateProxy.py
    testVP2RenderDelegateInteractiveWorkflows.py
    testVP2RenderDelegatePointInstanceSelection.py
    testVP2RenderDelegatePointInstancesPickMode.py
    testVP2RenderDelegatePrimPath.py
    testVP2RenderDelegateUSDPreviewSurface.py
    testVP2RenderDelegateConsolidation.py
    testVP2RenderDelegatePerInstanceInheritedData.py
    testVP2RenderDelegateBasisCurves.py
    testVP2RenderDelegatePoints.py
    testVP2RenderDelegateUsdCamera.py
    testVP2RenderDelegatePayloadBounds.py
)

if (MAYA_APP_VERSION VERSION_GREATER 2022)
    list(APPEND TEST_SCRIPT_FILES_LAMBERT
        testVP2RenderDelegateSelection.py
        testVP2RenderDelegateUsdSkel.py
        testVP2RenderDelegateMarqueeSelection.py
    )
endif()

if (MAYA_APP_VERSION VERSION_GREATER 2024)
    list(APPEND TEST_SCRIPT_FILES_LAMBERT
        testVP2RenderDelegateDrawModes.py
    )
endif()

if(MAYA_CAMERA_GIZMO_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES_LAMBERT
        testVP2RenderDelegateLights.py
        testVP2RenderDelegateCameras.py
    )
endif()

if (CMAKE_WANT_MATERIALX_BUILD)
    # We want modern OCIO for the MaterialX test:
    mayaUsd_get_unittest_target(target testVP2RenderDelegateMaterialX.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT testVP2RenderDelegateMaterialX.py
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"
            "MATERIALX_VERSION=${MaterialX_VERSION}"
            "MAYA_HAS_COLOR_MANAGEMENT_SUPPORT_API=${MAYA_HAS_COLOR_MANAGEMENT_SUPPORT_API}"
            "MAYAUSD_EXPORT_EXPANDED_COLORSPACE_ATTRIBUTE=1"
            "USD_HAS_MX_OPENPBR_SURFACE=${USD_HAS_MX_OPENPBR_SURFACE}"
            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Legacy lambert tests will force the use of the lamber fallback shaders
            "MAYAUSD_VP2_USE_LAMBERT_FALLBACK=true"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)

    # Test MaterialX deactivation:
    mayaUsd_get_unittest_target(target testVP2RenderDelegateNoMaterialX.py)
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT testVP2RenderDelegateNoMaterialX.py
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"
            "MATERIALX_VERSION=${MaterialX_VERSION}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Specify we only want preview surface rendering:
            "MAYAUSD_VP2_USE_ONLY_PREVIEWSURFACE=1"

            # Legacy lambert tests will force the use of the lamber fallback shaders
            "MAYAUSD_VP2_USE_LAMBERT_FALLBACK=true"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)
endif()

if(MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT)
    list(APPEND TEST_SCRIPT_FILES_LAMBERT
        testVP2RenderDelegateIsolateSelect.py
    )
endif()

if (MAYA_HAS_DISPLAY_LAYER_API)
    list(APPEND TEST_SCRIPT_FILES_LAMBERT
        testVP2RenderDelegateDisplayLayers.py
        )
endif()

foreach(script ${TEST_SCRIPT_FILES_LAMBERT})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Used in detecting whether there is support for volume light gizmos
            "UFE_VOLUME_LIGHTS_SUPPORT=${UFE_VOLUME_LIGHTS_SUPPORT}"

            # Legacy lambert tests will force the use of the lamber fallback shaders
            "MAYAUSD_VP2_USE_LAMBERT_FALLBACK=true"

            # Used in detecting whether there is support for UFE lights 2 API
            "UFE_LIGHTS2_SUPPORT=${UFE_LIGHTS2_SUPPORT}"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)
endforeach()


# Modern tests with maya 2024+ will use the standardSurface fallback vp2 shader fragments
set(TEST_SCRIPT_FILES_STANDARDSURFACE "")

list(APPEND TEST_SCRIPT_FILES_STANDARDSURFACE
    testVP2RenderDelegateDisplayColorsStandardSurface.py
)

foreach(script ${TEST_SCRIPT_FILES_STANDARDSURFACE})
    mayaUsd_get_unittest_target(target ${script})
    mayaUsd_add_test(${target}
        INTERACTIVE
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        PYTHON_SCRIPT ${script}
        ENV
            "MAYA_PLUG_IN_PATH=${CMAKE_INSTALL_PREFIX}/lib/maya"
            "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
            "MAYA_LIGHTAPI_VERSION=${MAYA_LIGHTAPI_VERSION}"

            # Maya uses a very old version of GLEW, so we need support for
            # pre-loading a newer version from elsewhere.
            "LD_PRELOAD=${ADDITIONAL_LD_PRELOAD}"

            # Used in detecting whether there is support for volume light gizmos
            "UFE_VOLUME_LIGHTS_SUPPORT=${UFE_VOLUME_LIGHTS_SUPPORT}"

            # Used in detecting whether there is support for UFE lights 2 API
            "UFE_LIGHTS2_SUPPORT=${UFE_LIGHTS2_SUPPORT}"
    )

    # Assign a CTest label to these tests for easy filtering.
    set_property(TEST ${target} APPEND PROPERTY LABELS vp2RenderDelegate)
endforeach()

# -----------------------------------------------------------------------------
# C++ unit tests
# -----------------------------------------------------------------------------
if(IS_WINDOWS AND CMAKE_WANT_MAYAUSD_VP2_USE_SCENE_INDEX)
    add_executable(testSubtreeScopingSceneIndex)

    target_sources(testSubtreeScopingSceneIndex
        PRIVATE
        main.cpp
        testSubtreeScopingSceneIndex.cpp
        # Compiled in rather than linked from mayaUsd, so the class needs no export.
        ${CMAKE_SOURCE_DIR}/lib/mayaUsd/render/vp2RenderDelegate/sceneIndex/subtreeScopingSceneIndex.cpp
    )

    mayaUsd_compile_config(testSubtreeScopingSceneIndex)

    target_include_directories(testSubtreeScopingSceneIndex
        SYSTEM PRIVATE
        ${PXR_INCLUDE_DIRS}
    )
    target_include_directories(testSubtreeScopingSceneIndex
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
    )

    target_compile_definitions(testSubtreeScopingSceneIndex
        PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    )

    target_link_libraries(testSubtreeScopingSceneIndex
        PRIVATE
        GTest::GTest
        gf
        hd
        hdsi
        sdf
        tf
        usd
        usdGeom
        usdImaging
        usdShade
    )

    mayaUsd_add_test(testSubtreeScopingSceneIndex
        COMMAND $<TARGET_FILE:testSubtreeScopingSceneIndex>
        ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
    )

    set_property(TEST testSubtreeScopingSceneIndex APPEND PROPERTY LABELS vp2RenderDelegate)

    add_executable(testSceneIndexProducer)

    target_sources(testSceneIndexProducer
        PRIVATE
        main.cpp
        testSceneIndexProducer.cpp
        # Compiled in rather than linked from mayaUsd, so the classes need no export.
        ${CMAKE_SOURCE_DIR}/lib/mayaUsd/render/vp2RenderDelegate/sceneIndexProducer.cpp
        ${CMAKE_SOURCE_DIR}/lib/mayaUsd/render/vp2RenderDelegate/sceneIndex/subtreeScopingSceneIndex.cpp
    )

    mayaUsd_compile_config(testSceneIndexProducer)

    target_include_directories(testSceneIndexProducer
        SYSTEM PRIVATE
        ${PXR_INCLUDE_DIRS}
    )
    target_include_directories(testSceneIndexProducer
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
    )

    target_compile_definitions(testSceneIndexProducer
        PRIVATE
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:TBB_USE_DEBUG>
        # Needed by Pixar's wrap_python.hpp
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_DEBUG_PYTHON>
        $<$<STREQUAL:${CMAKE_BUILD_TYPE},Debug>:BOOST_LINKING_PYTHON>
    )

    target_link_libraries(testSceneIndexProducer
        PRIVATE
        GTest::GTest
        gf
        hd
        hdsi
        hdx
        sdf
        tf
        usd
        usdGeom
        usdImaging
        usdShade
    )

    mayaUsd_add_test(testSceneIndexProducer
        COMMAND $<TARGET_FILE:testSceneIndexProducer>
        ENV
        "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
        "MAYA_LOCATION=${MAYA_LOCATION}"
    )

    set_property(TEST testSceneIndexProducer APPEND PROPERTY LABELS vp2RenderDelegate)
endif()
