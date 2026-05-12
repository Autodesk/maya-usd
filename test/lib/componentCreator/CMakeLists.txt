
if (AdskUsdComponentCreator_FOUND)
    # Install Component Creator tests split one test class per script.
    set(COMPONENT_CREATOR_TEST_SCRIPT_FILES
        testComponentCreatorLoadPlugin.py
        testComponentCreatorCreateComponentFromNodes.py
        testComponentCreatorCreateComponentFromUsdPrims.py
        testComponentCreatorCreateMultiVariantsComponentFromNodes.py
        testComponentCreatorCreateMultiVariantsComponentFromUsdPrims.py
        testComponentCreatorAddToComponentFromNodes.py
        testComponentCreatorAddToComponentFromUsdPrims.py
        testComponentCreatorDuplicateToComponent.py
        testComponentCreatorDeleteInComponent.py
        testComponentCreatorReparentInComponent.py
        testComponentCreatorRenameInComponent.py
        testComponentCreatorComponentStageInvariants.py
        testComponentCreatorAddPrimInComponent.py
    )

    foreach(script ${COMPONENT_CREATOR_TEST_SCRIPT_FILES})
        mayaUsd_get_unittest_target(target ${script})
        mayaUsd_add_test(${target}
            INTERACTIVE
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            PYTHON_SCRIPT ${script}
            ENV
                "LD_LIBRARY_PATH=${ADDITIONAL_LD_LIBRARY_PATH}"
                # The earlier Component Creator installs had a Maya plugin to initialize the CC
                # so we need to find the CC .mod file for the test.
                # This can be removed once we integrate the update CC which removed the Maya plugin
                # and we initialize the CC directly from MayaUsd. Having this MAYA_MODULE_PATH is
                # okay after the CC update since this path won't exist and therefore will be ignored.
                "MAYA_MODULE_PATH=${CMAKE_INSTALL_PREFIX}/../AdskUsdComponentCreator"
                "PXR_OVERRIDE_PLUGINPATH_NAME=${CMAKE_INSTALL_PREFIX}/lib/usd"
        )
        set_property(TEST ${target} APPEND PROPERTY LABELS MayaUsd)
    endforeach()
else()
    # If MAYAUSD_FORCE_CC_TEST is set to 1, fail the build when component creator is not found
    if (DEFINED ENV{MAYAUSD_FORCE_CC_TEST} AND "$ENV{MAYAUSD_FORCE_CC_TEST}" STREQUAL "1")
        message(FATAL_ERROR "MAYAUSD_FORCE_CC_TEST is set to 1 but AdskUsdComponentCreator was not found. ")
    endif()
endif()
