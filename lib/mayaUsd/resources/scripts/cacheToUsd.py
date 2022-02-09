#
# Copyright 2022 Autodesk
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

import mayaUsd
import maya.cmds as cmds

import re

def turnOnAnimation(textOptions):
    return re.sub(r'animation=.', 'animation=1', textOptions)

def getDefaultExportOptions():
    # Animation is always on for cache to USD.
    return turnOnAnimation(cmds.translator('USD Export', query=True, do=True))

def getCacheCreationOptions(exportOptions, cacheFile, cachePrimName,
                            payloadOrReference, listEditType,
                            variantSetName = None, variantName = None):

    defineInVariant = 0 if variantSetName is None else 1

    userArgs = mayaUsd.lib.Util.getDictionaryFromEncodedOptions(exportOptions)
    
    userArgs['rn_layer']              = cacheFile
    userArgs['rn_primName']           = cachePrimName
    userArgs['rn_defineInVariant']    = defineInVariant
    userArgs['rn_payloadOrReference'] = payloadOrReference
    userArgs['rn_listEditType']       = listEditType
    
    if defineInVariant:
        userArgs['rn_variantSetName'] = variantSetName
        userArgs['rn_variantName'] = variantName

    return userArgs
