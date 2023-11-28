# Copyright 2023 Autodesk
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

import mayaUsd.lib

from maya.OpenMaya import MGlobal

import os.path
import json


def getNiceAttributeName(ufeAttr, attrName):
    '''
    Convert the attribute name into  nice name.
    '''
    # Note: the uiname metadata comes from LookdevX and was used for connections.
    if hasattr(ufeAttr, 'displayName'):
        attrName = ufeAttr.displayName
    elif ufeAttr.hasMetadata("uiname"):
        attrName = str(ufeAttr.getMetadata("uiname"))
    return mayaUsd.lib.Util.prettifyName(attrName)


class AttributeCustomControl(object):
    '''
    Base class for attribute custom controls.
    Takes care of managing the attribute label.
    '''
    def __init__(self, ufeAttr, attrName, useNiceName):
        super(AttributeCustomControl, self).__init__()
        self.ufeAttr = ufeAttr
        self.attrName = attrName
        self.useNiceName = useNiceName

    def getAttributeUILabel(self, ufeAttr, attrName):
        '''
        Return the label to be used in the UI for the given attribute.
        '''
        return getNiceAttributeName(ufeAttr, attrName) if self.useNiceName else attrName
    
    def getUILabel(self):
        '''
        Return the label to be used in the UI for the attribute set on this object.
        '''
        return self.getAttributeUILabel(self.ufeAttr, self.attrName)
    
