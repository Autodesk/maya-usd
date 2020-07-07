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
function(init_usd)
    # Adjust PYTHONPATH, PATH
    if (DEFINED MAYAUSD_TO_USD_RELATIVE_PATH)
        set(USD_INSTALL_LOCATION "${CMAKE_INSTALL_PREFIX}/${MAYAUSD_TO_USD_RELATIVE_PATH}")
    else()
        set(USD_INSTALL_LOCATION ${PXR_USD_LOCATION})
    endif()

    mayaUsd_append_path_to_env_var("PYTHONPATH" "${USD_INSTALL_LOCATION}/lib/python")
    if(WIN32)
        mayaUsd_append_path_to_env_var("PATH" "${USD_INSTALL_LOCATION}/bin;${USD_INSTALL_LOCATION}/lib")
    endif()
endfunction()

init_usd()
