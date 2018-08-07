#-
#*****************************************************************************
# Copyright 2017 Autodesk, Inc. All rights reserved.
#
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by Autodesk, Inc. and are
# protected by Federal copyright law. They may not be disclosed to third
# parties or copied or duplicated in any form, in whole or in part, without
# the prior written consent of Autodesk, Inc.
#*****************************************************************************
#+

#------------------------------------------------------------------------------
#
# Creates an SHARED library named Maya.
#
function(init_maya)

    message( STATUS "Maya DevKit location = ${MAYA_DEVKIT_LOCATION}" )

    if(PEPTIDE_IS_WINDOWS)
        set(MAYAIMPORTLIB "${MAYA_DEVKIT_LOCATION}/lib/")
    endif()

	if(PEPTIDE_IS_OSX)
		set(MAYASHAREDLIB "${MAYA_DEVKIT_LOCATION}/Maya.app/Contents/MacOS/")
	else()
		set(MAYASHAREDLIB "${MAYA_DEVKIT_LOCATION}/lib/")
	endif()

    set(MAYASHAREDLIB "${MAYASHAREDLIB}" PARENT_SCOPE)

    set(MAYA_DEVKIT_INCLUDE_DIRS "${MAYA_DEVKIT_LOCATION}/include")
    set(MAYA_DEVKIT_INCLUDE_DIRS "${MAYA_DEVKIT_INCLUDE_DIRS}" PARENT_SCOPE)
    
    set(UFE_MAYA_DEVKIT_INCLUDE_DIRS "${MAYA_DEVKIT_LOCATION}/devkit/ufe/include")
    set(UFE_MAYA_DEVKIT_INCLUDE_DIRS "${UFE_MAYA_DEVKIT_INCLUDE_DIRS}" PARENT_SCOPE)
    
    set(UFE_MAYA_DEVKIT_LIB_DIR "${MAYA_DEVKIT_LOCATION}/devkit/ufe/lib")
    set(UFE_MAYA_DEVKIT_LIB_DIR "${UFE_MAYA_DEVKIT_LIB_DIR}" PARENT_SCOPE)

       
   set(maya_libraries OpenMaya
                      Foundation 
                      OpenMayaAnim 
                      OpenMayaFX
                      OpenMayaRender
                      OpenMayaUI
                      clew)

    add_custom_target( copy_mayaPrivate ALL )

    foreach( libname ${maya_libraries} )

        if(PEPTIDE_IS_WINDOWS)
            #dll are not part of the devkitBase so we use the .lib instead
            #set(MAYASHAREDLIB_NAME "${libname}${CMAKE_SHARED_LIBRARY_SUFFIX}")
            set(MAYASHAREDLIB_NAME "${libname}.lib")
        elseif(PEPTIDE_IS_OSX)
            set(MAYASHAREDLIB_NAME "lib${libname}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        elseif(PEPTIDE_IS_LINUX)
            set(MAYASHAREDLIB_NAME "lib${libname}${CMAKE_SHARED_LIBRARY_SUFFIX}")
        endif()

        find_file(
            MAYASHAREDLIB_FULLNAME ${MAYASHAREDLIB_NAME}
            PATHS ${MAYASHAREDLIB}
            DOC "Path to the maya library to use."
            NO_DEFAULT_PATH)

        if (NOT MAYASHAREDLIB_FULLNAME)
            message(FATAL_ERROR "Can't find maya library ${MAYASHAREDLIB_NAME} in ${MAYASHAREDLIB}")
        endif()

        if(PEPTIDE_IS_WINDOWS)
            set(SHAREDLIB_NAME "${PEPTIDE_OUTPUT_BIN_DIR}/${MAYASHAREDLIB_NAME}")
        else()
            set(SHAREDLIB_NAME "${PEPTIDE_OUTPUT_LIB_DIR}/${MAYASHAREDLIB_NAME}")
        endif()

        if(NEED_WINDOWS_PDB)
            string(REPLACE "${CMAKE_SHARED_LIBRARY_SUFFIX}" ".pdb" SHAREDLIB_PDB "${SHAREDLIB_NAME}")
            string(REPLACE "${CMAKE_SHARED_LIBRARY_SUFFIX}" ".pdb" MAYASHAREDLIB_PDB "${MAYASHAREDLIB_FULLNAME}")
            peptide_copy_file( copy_mayaPrivate "${MAYASHAREDLIB_PDB}" "${SHAREDLIB_PDB}" )
        endif()

        # Create the imported library target
        add_library(${libname} SHARED IMPORTED)
        set_property(TARGET ${libname} PROPERTY IMPORTED_LOCATION "${SHAREDLIB_NAME}")
        set_property(TARGET ${libname} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${MAYA_DEVKIT_INCLUDE_DIRS}")

        peptide_copy_file( ${libname} "${MAYASHAREDLIB_FULLNAME}" "${SHAREDLIB_NAME}" )

        if(PEPTIDE_IS_WINDOWS)
            set( MAYAIMPORTLIB_NAME "${libname}${CMAKE_IMPORT_LIBRARY_SUFFIX}" )
            unset( MAYAIMPORTLIB_FULLNAME CACHE )
            find_file(
                MAYAIMPORTLIB_FULLNAME ${MAYAIMPORTLIB_NAME}
                PATHS ${MAYAIMPORTLIB}
                DOC "Path to the maya library to use."
                NO_DEFAULT_PATH)
            if (NOT MAYAIMPORTLIB_FULLNAME)
                message(FATAL_ERROR "Can't fin maya library ${MAYAIMPORTLIB_NAME} in ${MAYAIMPORTLIB}")
            endif()

            set_property(TARGET ${libname} PROPERTY IMPORTED_IMPLIB "${MAYAIMPORTLIB_FULLNAME}")

            set(IMPORTLIB_NAME "${PEPTIDE_OUTPUT_LIB_DIR}/${MAYAIMPORTLIB_NAME}")
            peptide_copy_file( ${libname} "${MAYAIMPORTLIB_FULLNAME}" "${IMPORTLIB_NAME}" )
        endif()

        unset( MAYASHAREDLIB_FULLNAME CACHE )

    endforeach()

    if(PEPTIDE_IS_WINDOWS)
        set_property(TARGET clew PROPERTY INTERFACE_LINK_LIBRARIES "opengl32.lib")
    endif()

endfunction(init_maya)

# Using a function to scope variables and avoid polluting the global namespace
init_maya()
