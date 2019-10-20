#-
# =======================================================================
# Copyright 2018 Autodesk, Inc. All rights reserved.
#
# This computer source code and related instructions and comments are the
# unpublished confidential  and proprietary information of Autodesk, Inc.
# and are protected under applicable copyright and trade secret law. They 
# may not be disclosed to, copied  or used by any third party without the 
# prior written consent of Autodesk, Inc.
# =======================================================================
#+

#------------------------------------------------------------------------------
#
# Gets the Jinja2 and the dependant MarkupSafe python libraries from
# artifactory and set them up.
#
function(init_markupsafe)

    mayaUsd_find_python_module(markupsafe)

    if (NOT MARKUPSAFE_FOUND)
        if (NOT MARKUPSAFE_LOCATION)
            message(FATAL_ERROR "MARKUPSAFE_LOCATION not set")
        endif()

        set(MARKUPSAFE_ROOT "${MARKUPSAFE_LOCATION}/src")

        # Add MarkupSafe to the python path so that Jinja2 can run properly.
        mayaUsd_append_path_to_env_var("PYTHONPATH" "${MARKUPSAFE_ROOT}")
    endif()

endfunction()

function(init_jinja)

    mayaUsd_find_python_module(jinja2)

    if (NOT JINJA2_FOUND)
        if (NOT JINJA_LOCATION)
            message(FATAL_ERROR "JINJA_LOCATION not set")
        endif()

        set(JINJA_ROOT "${JINJA_LOCATION}")

        # Add Jinja2 to the python path so that usdGenSchemas can run properly.
        mayaUsd_append_path_to_env_var("PYTHONPATH" "${JINJA_ROOT}")
    endif()

endfunction()

init_markupsafe()
init_jinja()
