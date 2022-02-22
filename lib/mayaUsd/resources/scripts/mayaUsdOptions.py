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

import maya.cmds as cmds


def getOptionsText(varName, defaultOptions):
    """
    Retrieves the current options as text with column-separated key/value pairs.
    If the options don't exist, return the default options in the same text format.
    """
    if cmds.optionVar(exists=varName):
        return cmds.optionVar(query=varName)
    elif isinstance(defaultOptions, dict):
        return convertOptionsDictToText(defaultOptions)
    else:
        return defaultOptions
    

def setOptionsText(varName, optionsText):
    """
    Sets the current options as text with column-separated key/value pairs.
    """
    return cmds.optionVar(stringValue=(varName, optionsText))


def convertOptionsDictToText(optionsDict):
    """
    Converts options to text with column-separated key/value pairs.
    """
    optionsList = ['%s=%s' % (name, value) for name, value in optionsDict.items()]
    return ';'.join(optionsList)


def convertOptionsTextToDict(optionsText, defaultOptionsDict):
    """
    Converts options from text with column-separated key/value pairs to a dict.
    """
    # Initialize the result dict with the defaults, making a copy to avoid modifying
    # the given default dict.
    optionsDict = defaultOptionsDict.copy()

    # Parse each key/value pair that were extracted by splitting at columns.
    for opt in optionsText.split(';'):
        if '=' in opt:
            key, value = opt.split('=')
        else:
            key, value = opt, ""

        # Use the default values to try to convert the saved value to the correct type.
        if key in optionsDict:
            optionsDict[key] = _convertType(value, optionsDict[key])
        else:
            optionsDict[key] = value

    return optionsDict


def _convertType(valueToConvert, defaultValue):
    """
    Ensure the value has the correct expected type by converting it.
    Since the values are extracted from text, it is normal that they
    don't have the expected type right away.

    If the value cannot be converted, use the default value. This could
    happen if the optionVar got corrupted, for example from corrupted user
    prefs.
    """
    desiredType = type(defaultValue)
    if isinstance(valueToConvert, desiredType):
        return valueToConvert
    # Try to convert the value to the desired value.
    # We only support a subset of types to avoid problems,
    # for example trying to convert text to a list would
    # create a list of single letters.
    #
    # Use the default value if the data is somehow corrupted,
    # for example from corrupted user prefs.
    try:
        types = [str, int, float, bool]
        for t in types:
            if isinstance(defaultValue, t):
                return t(valueToConvert)
    except:
        return defaultValue
