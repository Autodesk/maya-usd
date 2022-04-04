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

import mayaUsdOptions

import re


def getDefaultExportOptions():
    """
    Retrieves the default export options used by cache-to-USD.
    """
    textOptions = cmds.translator('USD Export', query=True, do=True)
    textOptions += ";filterTypes=nurbsCurve"
    return textOptions


def getDefaultCacheCreationOptions():
    """
    Retrieves the default cache-to-USD options as a dictionary.
    """
    return createCacheCreationOptions(getDefaultExportOptions(), "", 'Cache1', 'Payload', 'Append', None, 'Cache')


def createCacheCreationOptions(exportOptions, cacheFile, cachePrimName,
                            payloadOrReference, listEditType,
                            variantSetName = None, variantName = None):
    """
    Creates the dict containing the export options and the cache-to-USD options.
    """

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


def _getVarName():
    """
    Retrieves the option var name used to save the cache-to-USD options.
    """
    return "mayaUsd_CacheToUSDOptions"


def saveCacheCreationOptions(optionsDict):
    """
    Saves the options dictionary into an option var.
    """
    optionsText = mayaUsdOptions.convertOptionsDictToText(optionsDict)
    mayaUsdOptions.setOptionsText(_getVarName(), optionsText)


def loadCacheCreationOptions():
    """
    Loads the cache-to-USD options from the option var and returns them as a dictionary.
    """
    optionsText = mayaUsdOptions.getOptionsText(_getVarName(), getDefaultExportOptions())
    return mayaUsdOptions.convertOptionsTextToDict(optionsText, getDefaultCacheCreationOptions())
