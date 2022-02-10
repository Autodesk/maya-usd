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

# These names should not be localized as Usd only accepts [a-z,A-Z] as valid characters.
kDefaultMayaReferencePrimName = 'MayaReference1'
kDefaultVariantSetName = 'Representation'
kDefaultVariantName = 'MayaReference'

def defaultMayaReferencePrimName():
    return kDefaultMayaReferencePrimName

def defaultVariantSetName():
    return kDefaultVariantSetName

def defaultVariantName():
    return kDefaultVariantName

class SetParentContext():
    '''Simple context helper to go up one parent level when exiting.'''
    def __init__(self, parent):
        cmds.setParent(parent)
        pass

    def __enter__(self):
        pass

    def __exit__(self, mytype, value, tb):
        cmds.setParent('..')

def pushOptionsUITemplate():
    '''Standardize the look of the options UI.

    Python translation of fileOptions.mel:pushOptionsUITemplate(),
    which is not a global proc.
    '''
    if not cmds.uiTemplate('optionsTemplate', exists=True):
        cmds.uiTemplate('optionsTemplate')

        cmds.frameLayout(defineTemplate='optionsTemplate',
                         collapsable=True,
                         collapse=False,
                         labelVisible=True,
                         labelIndent=5,
                         marginWidth=5,
                         marginHeight=5)

        cmds.columnLayout(defineTemplate='optionsTemplate',
                          adjustableColumn=True)

    cmds.setUITemplate('optionsTemplate', pushTemplate=True)

