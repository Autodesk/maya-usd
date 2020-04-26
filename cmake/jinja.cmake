#
# Copyright 2020 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
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

        set(JINJA_ROOT "${JINJA_LOCATION}/src")

        # Add Jinja2 to the python path so that usdGenSchemas can run properly.
        mayaUsd_append_path_to_env_var("PYTHONPATH" "${JINJA_ROOT}")
    endif()

endfunction()

init_markupsafe()
init_jinja()
