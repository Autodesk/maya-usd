# usd api
include_directories(${PXR_INCLUDE_DIRS})

# usd cmake config helpers
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_LIST_DIR}/defaults
    ${CMAKE_CURRENT_LIST_DIR}/modules
    ${CMAKE_CURRENT_LIST_DIR}/macros)

# Verbose Makefiles by default.
set(CMAKE_VERBOSE_MAKEFILE OFF)

# CXXDefaults will set a variety of variables for the project.
# Consume them here.  This is an effort to keep the most common
# build files readable.
include(CXXDefaults)
add_definitions(${_PXR_CXX_DEFINITIONS})
set(CMAKE_CXX_FLAGS
    ${_PXR_CXX_FLAGS})


# needed by _install_python function
set(PYTHON_EXECUTABLE ${python_ROOT}/bin/python)

include(ProjectDefaults)
# to get pxr_shared_library function
include(Public)

# copy pxr_setup_python to add namespace package definition
function(al_setup_python)
    get_property(pxrPythonModules GLOBAL PROPERTY PXR_PYTHON_MODULES)

    # A new list where each python module is quoted
    set(converted "")
    foreach(module ${pxrPythonModules})
        list(APPEND converted "'${module}'")
    endforeach()

    # Join these with a ', '
    string(REPLACE ";" ", " pyModulesStr "${converted}")

    # Install a pxr __init__.py with an appropriate __all__
    _get_install_dir(lib/python/pxr installPrefix)
    install(CODE
        "file(WRITE \"${CMAKE_INSTALL_PREFIX}/${installPrefix}/__init__.py\" \"try:\n\t__import__('pkg_resources').declare_namespace(__name__)\nexcept:\n\tpass\n__all__ = [${pyModulesStr}]\n\")"
    )
endfunction()

macro(al_get_schema_sources pathToSource cppFiles pymoduleCppFiles publicHeaderFiles)
    #Determine the Python Wrap files
    FILE(GLOB ${pymoduleCppFiles} "${pathToSource}/wrap*.cpp")
    list(APPEND ${pymoduleCppFiles} "${pathToSource}/module.cpp")

    #Determine CPP files
    FILE(GLOB ${cppFiles} "${pathToSource}/*.cpp")
    list(REMOVE_ITEM ${cppFiles} ${${pymoduleCppFiles}})
    list(REMOVE_ITEM ${cppFiles} "${pathToSource}/module.cpp")

    #Determine Header files
    FILE(GLOB ${publicHeaderFiles} "${pathToSource}/*.h")
endmacro()

function(al_plugInfo_subst libName plugInfoPath outputName)
    # Generate plugInfo.json files from a template. Note that we can't use
    # the $<TARGET_FILE_NAME:tgt> generator expression here because 
    # configure_file will run at configure time while the generators will only
    # run after.
    set(libFile ${CMAKE_SHARED_LIBRARY_PREFIX}${libName}${CMAKE_SHARED_LIBRARY_SUFFIX})

    # The root resource directory is in $PREFIX/share/usd/$LIB/resource but the 
    # libs are actually in $PREFIX/lib. The lib path can then be specified
    # relatively as below.
    set(PLUG_INFO_LIBRARY_PATH "../../../../lib/${libFile}")
    set(PLUG_INFO_RESOURCE_PATH "resources")
    set(PLUG_INFO_ROOT "..")

    configure_file(
        ${plugInfoPath}
        ${CMAKE_CURRENT_BINARY_DIR}/${outputName}
    )
endfunction(al_plugInfo_subst)
