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

import mayaUsd

import re

def defaultOptionBoxSize():
    ''' Default size copied from getOptionBox.mel: defaultOptionBoxSize()
        as that proc is not global.'''
    return 546, 350

def setAnimateOption(nodeName, textOptions):
    """
    Adjusts the export options to fill the animate value based on the node or subnode being animated.
    """
    animated = int(mayaUsd.lib.PrimUpdaterManager.isAnimated(nodeName))
    animateOption = 'animation={animated}'.format(animated=animated)
    if 'animation=' in textOptions:
        return re.sub(r'animation=[^;]+', animateOption, textOptions)
    else:
        return textOptions + ';' + animateOption


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
    optionsList = ['%s=%s' % (name, _convertValueToText(value)) for name, value in optionsDict.items()]
    return ';'.join(optionsList)


def _convertValueToText(value):
    # Unfortunately, for historical reasons, text lists have used
    # comma-separated values, while floating-point used space-separated values.
    if isinstance(value,list):
        # Note: empty list must create empty text.
        if not value:
            return ''
        if isinstance(value[0], str):
            sep = ','
        else:
            sep = ' '
        return sep.join([_convertValueToText(v) for v in value])
    elif isinstance(value,bool):
        return str(int(value))
    else:
        return str(value)


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

        # Use the guide values to try to convert the saved value to the correct type.
        if key in optionsDict:
            optionsDict[key] = _convertTextToType(value, optionsDict[key])
        else:
            optionsDict[key] = value

    return optionsDict


def _convertTextToType(valueToConvert, defaultValue, desiredType=None):
    """
    Ensure the value has the correct expected type by converting it.
    Since the values are extracted from text, it is normal that they
    don't have the expected type right away.

    If the value cannot be converted, use the default value. This could
    happen if the optionVar got corrupted, for example from corrupted user
    prefs.
    """
    if not desiredType:
        desiredType = type(defaultValue)
    if isinstance(valueToConvert, desiredType):
        return valueToConvert
    # Try to convert the value to the desired type.
    # We only support a subset of types to avoid problems,
    # for example trying to convert text to a list would
    # create a list of single letters.
    try:
        textConvertibleTypes = [str, int, float]
        if desiredType in textConvertibleTypes:
            return desiredType(valueToConvert)
        elif desiredType == bool:
            if valueToConvert.lower() == 'true':
                return True
            if valueToConvert.lower() == 'false':
                return False
            return bool(int(valueToConvert))
    except:
        pass

    # Verify to convert list values by converting each element.
    # Unfortunately, for historical reasons, text lists have used
    # comma-separated values, while floating-point used space-separated values.
    try:
        if isinstance(defaultValue, list):
            if ',' in valueToConvert:
                values = valueToConvert.split(',')
                desiredType = str
            else:
                values = valueToConvert.split()
                desiredType = float
            if len(defaultValue):
                desiredType = type(defaultValue[0])
            convertedValues = []
            for value in values:
                value = value.strip()
                if not value:
                    continue
                convertedValue = _convertTextToType(value.strip(), None, desiredType)
                if convertedValue is None:
                    continue
                convertedValues.append(convertedValue)
            return convertedValues
    except:
        pass

    # Use the default value if the data is somehow corrupted,
    # for example from corrupted user prefs.
    return defaultValue
