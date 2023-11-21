# Module to find Qt in Maya devkit.
#
# This module searches for the necessary Qt files in the Maya devkit.
#
# It relies on Maya being found first: find_package(Maya)
# as it uses variables set there.
#
# Variables that will be defined:
#    Maya_Qt_FOUND                  Defined if Qt has been found
#    Qt{$QT_MAJOR_VERSION}_FOUND    Defined if Qt has been found
#    MAYA_QT_INC_DIR                The include folder where Qt header files can be found
#    QT_MOC_EXECUTABLE              Path to Qt moc executable
#    QT_UIC_EXECUTABLE              Path to Qt uic executable
#    QT_RCC_EXECUTABLE              Path to Qt rcc executable
#    MAYA_QT_VERSION                The version of Qt in the Maya devkit
#

# List of directories to search for Maya Qt headers/libraries/executables.
set(MAYA_DIRS_TO_SEARCH
    ${MAYA_DEVKIT_LOCATION}
    $ENV{MAYA_DEVKIT_LOCATION}
    ${MAYA_LOCATION}
    $ENV{MAYA_LOCATION}
)

# Find the directory paths to the Qt headers (using the input components list).
foreach(MAYA_DIR_TO_SEARCH ${MAYA_DIRS_TO_SEARCH})
    file(TO_CMAKE_PATH ${MAYA_DIR_TO_SEARCH} qt_root_dir)

    # Make sure we can find the header files for each Qt component.
    set(Maya_Qt_Headers_FOUND TRUE)  # Assume we will find them all.
    foreach(Maya_Qt_Component ${Maya_Qt_FIND_COMPONENTS})
        string(TOUPPER ${Maya_Qt_Component} MAYA_QT_COMPONENT)
        string(TOLOWER ${Maya_Qt_Component} maya_qt_component)
        unset(MAYA_QT_VERSION_FILE)
        file(GLOB_RECURSE MAYA_QT_VERSION_FILE "${qt_root_dir}/include/*/qt${maya_qt_component}version.h")
        if (NOT MAYA_QT_VERSION_FILE)
            set(Maya_Qt_Headers_FOUND FALSE)
            break()     # We could not find the headers in this search path (try the next).
        endif()
        # Get the include directory for this component.
        get_filename_component(MAYA_QT_${MAYA_QT_COMPONENT}_DIR ${MAYA_QT_VERSION_FILE} DIRECTORY)
    endforeach()

    if (Maya_Qt_Headers_FOUND AND MAYA_QT_${MAYA_QT_COMPONENT}_DIR)
        # Get the base Qt include path (we can simply use the last component found as they are
        # all in the same base path).
        get_filename_component(MAYA_QT_INC_DIR ${MAYA_QT_${MAYA_QT_COMPONENT}_DIR} DIRECTORY)
        break() # We found all the headers in this search path.
    endif()
endforeach()

# Find Qt utilities.
set(Maya_Qt_Utilities "moc;uic;rcc")
foreach(Maya_Qt_Utility ${Maya_Qt_Utilities})
    string(TOUPPER ${Maya_Qt_Utility} MAYA_QT_UTILITY)
    find_program(QT_${MAYA_QT_UTILITY}_EXECUTABLE
        NAMES
            ${Maya_Qt_Utility}
        PATHS
            ${MAYA_DEVKIT_LOCATION}
            $ENV{MAYA_DEVKIT_LOCATION}
            ${MAYA_LOCATION}
            $ENV{MAYA_LOCATION}
            ${MAYA_BASE_DIR}
        PATH_SUFFIXES
            devkit/bin
            bin
        NO_DEFAULT_PATH
        DOC
            "Maya Qt utility"
    )
endforeach()

if (QT_UIC_EXECUTABLE)
    # Get the Qt version based on UIC
    execute_process(COMMAND ${QT_UIC_EXECUTABLE} "--version" OUTPUT_VARIABLE QT_UIC_VERSION)
    STRING(REPLACE "." " " QT_UIC_VERSION ${QT_UIC_VERSION})
    STRING(REPLACE " " ";" QT_UIC_VERSION ${QT_UIC_VERSION})
    LIST(GET QT_UIC_VERSION 1 QT_VERSION_MAJOR)
    LIST(GET QT_UIC_VERSION 2 QT_VERSION_MINOR)
    LIST(GET QT_UIC_VERSION 3 QT_PATCH_LEVEL)
    set(MAYA_QT_VERSION ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_PATCH_LEVEL})
    add_executable(Qt${QT_VERSION_MAJOR}::uic IMPORTED GLOBAL)
    set_target_properties(Qt${QT_VERSION_MAJOR}::uic PROPERTIES IMPORTED_LOCATION ${QT_UIC_EXECUTABLE})
endif()

if (QT_MOC_EXECUTABLE)
    add_executable(Qt${QT_VERSION_MAJOR}::moc IMPORTED GLOBAL)
    set_target_properties(Qt${QT_VERSION_MAJOR}::moc PROPERTIES IMPORTED_LOCATION ${QT_MOC_EXECUTABLE})
endif()

if (QT_RCC_EXECUTABLE)
    add_executable(Qt${QT_VERSION_MAJOR}::rcc IMPORTED GLOBAL)
    set_target_properties(Qt${QT_VERSION_MAJOR}::rcc PROPERTIES IMPORTED_LOCATION ${QT_RCC_EXECUTABLE})
endif()

# Find the Maya Qt libraries (using the input components list).
foreach(Maya_Qt_LIB ${Maya_Qt_FIND_COMPONENTS})
    string(TOUPPER ${Maya_Qt_LIB} MAYA_QT_LIB)
    set(QT_NAME_TO_FIND "Qt${QT_VERSION_MAJOR}${Maya_Qt_LIB}")
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        # When in debug we sometimes need to modify the names.
        if (IS_WINDOWS)
            string(APPEND QT_NAME_TO_FIND "d")         # extra 'd' at end
        elseif (IS_MACOSX)
            string(APPEND QT_NAME_TO_FIND "_debug")    # extra '_debug' at end
        endif()
    endif()
    find_library(MAYA_QT${MAYA_QT_LIB}_LIBRARY
        NAMES
            ${QT_NAME_TO_FIND}
        PATHS
            ${MAYA_LIBRARY_DIR}
        NO_DEFAULT_PATH
        DOC
            "Maya Qt library"
    )
    if (MAYA_QT${MAYA_QT_LIB}_LIBRARY)
        # Add each Qt library and set the target properties.
        set(__target_name "Qt${QT_VERSION_MAJOR}::${Maya_Qt_LIB}")
        add_library(${__target_name} SHARED IMPORTED GLOBAL)
        set_target_properties(${__target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MAYA_QT_${MAYA_QT_LIB}_DIR};${MAYA_QT_INC_DIR}")
        set_target_properties(${__target_name} PROPERTIES IMPORTED_CONFIGURATIONS RELEASE) # RELEASE? 
        set_target_properties(${__target_name} PROPERTIES IMPORTED_LOCATION "${MAYA_QT${MAYA_QT_LIB}_LIBRARY}")
        if(IS_WINDOWS)
            set_target_properties(${__target_name} PROPERTIES IMPORTED_IMPLIB "${MAYA_QT${MAYA_QT_LIB}_LIBRARY}")
            set_target_properties(${__target_name} PROPERTIES INTERFACE_LINK_LIBRARIES "${__target_name}")
        endif()
        if(${QT_VERSION_MAJOR} VERSION_EQUAL 6)
            # Add extra compile options required for Qt6.
            set_target_properties(${__target_name} PROPERTIES
                INTERFACE_COMPILE_FEATURES "cxx_std_17")   # Comes from Qt6::Platform
            if (IS_WINDOWS)
                # Comes from Qt6::Platform
                set_target_properties(${__target_name} PROPERTIES
                    INTERFACE_COMPILE_OPTIONS  "\$<\$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus;/permissive->")
            endif()
        endif()

        # Add a versionless target.
        set(__versionless_target_name "Qt::${Maya_Qt_LIB}")
        if (TARGET ${__target_name} AND NOT TARGET ${__versionless_target_name})
            add_library(${__versionless_target_name} INTERFACE IMPORTED)
            set_target_properties(${__versionless_target_name} PROPERTIES INTERFACE_LINK_LIBRARIES ${__target_name})
        endif()
    endif()
endforeach()

# Set the Qt library dependencies (special cases we know about).
if (IS_LINUX AND TARGET Qt${QT_VERSION_MAJOR}::Core)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::Core APPEND PROPERTY INTERFACE_COMPILE_OPTIONS -fPIC)
    set_property(TARGET Qt${QT_VERSION_MAJOR}::Core PROPERTY INTERFACE_COMPILE_FEATURES cxx_decltype)
endif()
if (TARGET Qt${QT_VERSION_MAJOR}::Widgets)
    set_target_properties(Qt${QT_VERSION_MAJOR}::Widgets PROPERTIES INTERFACE_LINK_LIBRARIES "Qt${QT_VERSION_MAJOR}::Gui;Qt${QT_VERSION_MAJOR}::Core")
endif()
if (TARGET Qt${QT_VERSION_MAJOR}::Gui)
    set_target_properties(Qt${QT_VERSION_MAJOR}::Gui PROPERTIES INTERFACE_LINK_LIBRARIES "Qt${QT_VERSION_MAJOR}::Core")
endif()

# Handle the QUIETLY and REQUIRED arguments and set UFE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)

list(APPEND Maya_QtRequiredVars "MAYA_QT_INC_DIR" "QT_MOC_EXECUTABLE" "QT_UIC_EXECUTABLE" "QT_RCC_EXECUTABLE")
foreach(Maya_Qt_LIB ${Maya_Qt_FIND_COMPONENTS})
    string(TOUPPER ${Maya_Qt_LIB} MAYA_QT_LIB)
    list(APPEND Maya_QtRequiredVars "MAYA_QT${MAYA_QT_LIB}_LIBRARY")
endforeach()

find_package_handle_standard_args(Maya_Qt
    REQUIRED_VARS
        ${Maya_QtRequiredVars}
    VERSION_VAR
        MAYA_QT_VERSION
)

if (Maya_Qt_FOUND)
    set(Qt${QT_VERSION_MAJOR}_FOUND TRUE)
    message(STATUS "Using Maya Qt version ${MAYA_QT_VERSION}")
    message(STATUS "  Qt include path  : ${MAYA_QT_INC_DIR}")
    message(STATUS "  moc executable   : ${QT_MOC_EXECUTABLE}")
    message(STATUS "  uic executable   : ${QT_UIC_EXECUTABLE}")
    message(STATUS "  rcc executable   : ${QT_RCC_EXECUTABLE}")
    foreach(Maya_Qt_LIB ${Maya_Qt_FIND_COMPONENTS})
        string(TOUPPER ${Maya_Qt_LIB} MAYA_QT_LIB)
        message(STATUS "  Qt ${Maya_Qt_LIB} library: ${MAYA_QT${MAYA_QT_LIB}_LIBRARY}")
    endforeach()
endif()
