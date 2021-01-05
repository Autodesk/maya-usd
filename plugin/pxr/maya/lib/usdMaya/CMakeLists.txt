set(PXR_PACKAGE usdMaya)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        ar
        gf
        kind
        sdf
        tf
        usd
        usdGeom
        usdUtils
        vt
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaAnim_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}
        mayaUsd

    INCLUDE_DIRS
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        editUtil
        translatorModelAssembly

        exportCommand
        exportTranslator
        importCommand
        importTranslator
        listShadingModesCommand
        proxyShape
        referenceAssembly

    PRIVATE_CLASSES
        readJobWithSceneAssembly
        instancerShapeAdapterWithSceneAssembly

    CPPFILES
        readJob_ImportWithProxies.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapAssembly.cpp
        wrapEditUtil.cpp

    PYMODULE_FILES
        __init__.py
        AEpxrUsdReferenceAssemblyTemplate.py
        userExportedAttributesUI.py

    RESOURCE_FILES
        AEpxrUsdProxyShapeTemplate.mel
        out_pxrUsdProxyShape.xpm
        out_pxrUsdReferenceAssembly.xpm
        usdMaya.mel
        usdTranslatorExport.mel
        usdTranslatorImport.mel

    DISABLE_PRECOMPILED_HEADERS
)

pxr_test_scripts(
        testenv/testUsdExportAssembly.py
        testenv/testUsdExportAssemblyEdits.py
        # testUsdExportPackage input file PackageTest.ma has a requirement on
        # pxrUsdReferenceAssembly, and therefore cannot be moved to the core.
        testenv/testUsdExportPackage.py
        # testUsdExportPointInstancer uses MFnAssembly (Maya scene assembly
        # function set), and therefore cannot be moved to the core.
        testenv/testUsdExportPointInstancer.py
        testenv/testUsdImportAsAssemblies.py
        testenv/testUsdImportNestedAssemblyAnimation.py
        testenv/testUsdMayaGetVariantSetSelections.py
        testenv/testUsdMayaModelKindProcessor.py
        testenv/testUsdMayaReadWriteUtils.py
        testenv/testUsdMayaReferenceAssemblyEdits.py
        testenv/testUsdMayaUserExportedAttributes.py
        testenv/testUsdMayaXformStack.py
        testenv/testUsdReferenceAssemblyChangeRepresentations.py
        testenv/testUsdReferenceAssemblySelection.py
)

# Note - we set up a maya profile directory ($MAYA_APP_DIR) that's empty, so we
# can ensure we test with a default profile - had some crashes with 2017 when
# using an existing user profile (due to some problems with color management
# settings)
# Also, maya creates the $MAYA_APP_DIR on demand, so we don't need to bother
# making the directory ourselves.  We use <PXR_TEST_DIR>, which is expanded
# to the absolute path of the temporary folder tests are run in, because:
#   A) we can't use relative paths ("./maya_profile"), because maya in
#      windows will error
#   B) we don't know the absolute path to the test directory at cmake-compile
#      time

# MAYA-96273 (closed as by design) says that to obtain correct mayapy exit
# codes starting with Maya 2018.4, the MAYA_NO_STANDALONE_ATEXIT environment
# variable must be defined.  Otherwise, mayapy unconditionally exits with 0
# (success), which completely masks test failures.

set(TEST_INSTALL_PREFIX "${PXR_INSTALL_PREFIX}")

pxr_install_test_dir(
    SRC testenv/UsdExportAssemblyTest
    DEST testUsdExportAssembly
)
pxr_register_test(testUsdExportAssembly
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdExportAssembly"
    TESTENV testUsdExportAssembly
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportAssemblyEditsTest
    DEST testUsdExportAssemblyEdits
)
pxr_register_test(testUsdExportAssemblyEdits
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdExportAssemblyEdits"
    TESTENV testUsdExportAssemblyEdits
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportPackage
    DEST testUsdExportPackage
)
pxr_register_test(testUsdExportPackage
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdExportPackage"
    TESTENV testUsdExportPackage
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdExportPointInstancerTest
    DEST testUsdExportPointInstancer
)
pxr_register_test(testUsdExportPointInstancer
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdExportPointInstancer"
    TESTENV testUsdExportPointInstancer
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportAsAssemblies
    DEST testUsdImportAsAssemblies
)
pxr_register_test(testUsdImportAsAssemblies
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdImportAsAssemblies"
    TESTENV testUsdImportAsAssemblies
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdImportNestedAssemblyAnimationTest
    DEST testUsdImportNestedAssemblyAnimation
)
pxr_register_test(testUsdImportNestedAssemblyAnimation
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdImportNestedAssemblyAnimation"
    TESTENV testUsdImportNestedAssemblyAnimation
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaGetVariantSetSelectionsTest
    DEST testUsdMayaGetVariantSetSelections
)
pxr_register_test(testUsdMayaGetVariantSetSelections
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdMayaGetVariantSetSelections"
    TESTENV testUsdMayaGetVariantSetSelections
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaModelKindProcessorTest
    DEST testUsdMayaModelKindProcessor
)
pxr_register_test(testUsdMayaModelKindProcessor
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdMayaModelKindProcessor"
    TESTENV testUsdMayaModelKindProcessor
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_register_test(testUsdMayaReadWriteUtils
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdMayaReadWriteUtils"
    TESTENV testUsdMayaReadWriteUtils
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaReferenceAssemblyEdits
    DEST testUsdMayaReferenceAssemblyEdits
)
pxr_register_test(testUsdMayaReferenceAssemblyEdits
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdMayaReferenceAssemblyEdits"
    TESTENV testUsdMayaReferenceAssemblyEdits
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdMayaUserExportedAttributesTest
    DEST testUsdMayaUserExportedAttributes
)
pxr_register_test(testUsdMayaUserExportedAttributes
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdMayaUserExportedAttributes"
    TESTENV testUsdMayaUserExportedAttributes
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
        MAYAUSD_EXPORT_MAP1_AS_PRIMARY_UV_SET=1
)

pxr_install_test_dir(
    SRC testenv/UsdReferenceAssemblyChangeRepresentationsTest
    DEST testUsdReferenceAssemblyChangeRepresentations
)
pxr_register_test(testUsdReferenceAssemblyChangeRepresentations
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdReferenceAssemblyChangeRepresentations"
    TESTENV testUsdReferenceAssemblyChangeRepresentations
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_install_test_dir(
    SRC testenv/UsdReferenceAssemblySelectionTest
    DEST testUsdReferenceAssemblySelection
)
pxr_register_test(testUsdReferenceAssemblySelection
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdReferenceAssemblySelection"
    TESTENV testUsdReferenceAssemblySelection
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)

pxr_register_test(testUsdMayaXformStack
    CUSTOM_PYTHON ${MAYA_PY_EXECUTABLE}
    COMMAND "${TEST_INSTALL_PREFIX}/tests/testUsdMayaXformStack"
    TESTENV testUsdMayaXformStack
    ENV
        MAYA_PLUG_IN_PATH=${TEST_INSTALL_PREFIX}/maya/plugin
        MAYA_SCRIPT_PATH=${TEST_INSTALL_PREFIX}/maya/lib/usd/usdMaya/resources
        MAYA_DISABLE_CIP=1
        MAYA_NO_STANDALONE_ATEXIT=1
        MAYA_APP_DIR=<PXR_TEST_DIR>/maya_profile
)
