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


def getOptionsText(varName, defaultOptionsDict):
    """
    Retrieves the current options as text with column-separated key/value pairs.
    If the options don't exist, return the default options in the same text format.
    """
    if cmds.optionVar(exists=varName):
        return cmds.optionVar(query=varName)
    else:
        return convertOptionsDictToText(defaultOptionsDict)
    

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
