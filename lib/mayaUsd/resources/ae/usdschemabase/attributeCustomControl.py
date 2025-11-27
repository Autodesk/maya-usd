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
from mayaUsd import ufe as mayaUsdUfe

from maya.OpenMaya import MGlobal

import os.path
import json

# List of extra functions used when asked to provide an attribute nice name.
_niceAttributeNameFuncs = []

# A client can add a function to be called when asked for an attribute nice name.
# That function will be called from "getNiceAttributeName" below giving the
# client the ability to provide a nice name for the attribute.
#
# Your function will receive as input two params:
# - ufe attribute
# - attribute name
# And should return the adjusted name or None if no adjustment was made.
def prependNiceAttributeNameFunc(niceNameFunc):
    global _niceAttributeNameFuncs
    _niceAttributeNameFuncs.insert(0, niceNameFunc)

# Used to remove any function added with the "prependNiceAttributeNameFunc()" helper
# above. For example when unloading your plugin.
def removeAttributeNameFunc(niceNameFunc):
    global _niceAttributeNameFuncs
    if niceNameFunc in _niceAttributeNameFuncs:
        _niceAttributeNameFuncs.remove(niceNameFunc)

def getNiceAttributeName(ufeAttr, attrName):
    '''
    Convert the attribute name into a nice name.
    '''
    # Note: the uiname metadata comes from LookdevX and was used for connections.
    if hasattr(ufeAttr, 'displayName'):
        attrName = ufeAttr.displayName
    elif hasattr(ufeAttr, "hasMetadata") and ufeAttr.hasMetadata("uiname"):
        attrName = str(ufeAttr.getMetadata("uiname"))

    try:
        # If there are any nice attribute name functions registered call them now.
        # this gives clients the ability to inject some nice naming.
        global _niceAttributeNameFuncs
        for fn in _niceAttributeNameFuncs:
            tempName = fn(ufeAttr, attrName)
            if tempName:
                attrName = tempName
    except Exception as ex:
        # Do not let any of the callback failures affect our template.
        print('Failed to call AE nice naming callback for %s: %s' % (attrName, ex))

    # Finally use our internal MayaUsd nice naming function.
    return mayaUsdUfe.prettifyName(attrName)

def cleanAndFormatTooltip(s):
    if not s:
        return s

    # Remove leading/trailing whitespace.
    # Note: don't use any html tags in this string since it will also be used for the
    #       statusbar message which will just print the tags (rather than render them).
    lines = s.splitlines()
    stripped = [line.strip() for line in lines]
    return '\n'.join(stripped)

class AttributeCustomControl(object):
    '''
    Base class for attribute custom controls.
    Takes care of managing the attribute label.
    '''
    def __init__(self, ufeAttr, attrName, useNiceName, label=None):
        super(AttributeCustomControl, self).__init__()
        self.ufeAttr = ufeAttr
        self.attrName = attrName
        self.useNiceName = useNiceName
        self.label = label

    def getAttributeUILabel(self, ufeAttr, attrName):
        '''
        Return the label to be used in the UI for the given attribute.
        '''
        return getNiceAttributeName(ufeAttr, attrName) if self.useNiceName else attrName
    
    def getUILabel(self):
        '''
        Return the label to be used in the UI for the attribute set on this object.
        '''
        if self.label:
            return self.label
        return self.getAttributeUILabel(self.ufeAttr, self.attrName)
