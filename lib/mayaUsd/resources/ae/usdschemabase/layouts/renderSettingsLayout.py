# Copyright 2025 Autodesk
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

import collections
import re
import ufe
import maya.cmds as cmds
import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe
import maya.internal.common.ufe_ae.template as ufeAeTemplate

from pxr import Vt

class AERenderSettingsLayout(object):
    '''
    This class takes care of sorting attributes that use uifolder and uiorder metadata and to prepare
    a data structure that has the exact sequence of layouts and attributes necessary to properly
    lay out all the attributes in the right folders.
    '''
    # A named tuple for attribute information:
    _AttributeInfo = collections.namedtuple("_AttributeInfo", ["uiorder", "name", "uifolder"])
    # A string splitter for the most commonly used separators:
    _groupSplitter = re.compile(r"[/|\;:]")
    # Valid uiorder:
    _isDecimal = re.compile(r"^[0-9]+$")

    class Group(object):
        """Base class to return layout information. The list of items can contain subgroups."""
        def __init__(self, name, root=False):
            self._name = name
            self._items = []
            self._root = root
        @property
        def root(self):
            return self._root
        @property
        def name(self):
            return self._name
        @property
        def items(self):
            return self._items
        
        def get(self, name):
            if name in [item.name for item in self._items]:
                for item in self._items:
                    if item.name == name:
                        return item
            else:
                return None

        def addItem(self, item):
            self._items.append(item)


    def __init__(self, ufeSceneItem, template):
        self.item = ufeSceneItem
        self.prim = mayaUsdUfe.ufePathToPrim(ufe.PathString.string(self.item.path()))
        self._properties = []
        self._attributeInfoList = []
        self._canCompute = True
        self._attributeLayout = AERenderSettingsLayout.Group(mayaUsdUfe.prettifyName(self.item.nodeType()), root=True)
        self.template = template
        self.tabbar = None

        # Initialize the property groups
        self.propertyGroups = {
            'Settings': [
                'includedPurposes',
                'materialBindingPurposes',
                'products',
                'renderingColorSpace',
            ],
            'Base': [
                'resolution',
                'pixelAspectRatio',
                'aspectRatioConformPolicy',
                'dataWindowNDC',
                'disableMotionBlur',
                'disableDepthOfField',
                'camera',
                'instantaneousShutter',
            ],
            'Product': [
                'orderedVars',
                'productName',
                'productType',
            ]
        }

        self.parseRenderSettingsAttributes()

    def createRendererTabsLayout(self):
        """Create the tab layout for the render settings section"""

        def _createGroup(groupName, items):
            
            with ufeAeTemplate.Layout(self, groupName, collapse=False):
                for i in items:
                    if isinstance(i, AERenderSettingsLayout.Group):
                        _createGroup(i.name, i.items)
                    else:
                        if self.template.attrs.hasAttribute(i):
                            self.template.addControls([i])

        # Create the callback context/data (empty).
        cbContext = {
            'ufe_path_string' : ufe.PathString.string(self.item.path()),
        }
        cbContextDict = Vt._ReturnDictionary(cbContext)
        cbDataDict = Vt._ReturnDictionary({})

        cmds.setUITemplate('attributeEditorTemplate', pst=True)

        layout = ufeAeTemplate.Layout(self, "", collapse=False)
        if cmds.about(apiVersion=True) >= 20270100:
            # TabLayout introduced in Maya 2027.1
            layout = ufeAeTemplate.TabLayout(self)
        
        with layout:            
            for item in self._attributeLayout.items:
                if isinstance(item, AERenderSettingsLayout.Group):
                    _createGroup(item.name, item.items)
                else:
                    if self.template.attrs.attribute(item):
                        self.template.addControls([item])
            
            # Trigger the callback which will give other plugins the opportunity
            # to add controls to our AE template.
            try:
                # cbDataDict = mayaUsdLib.triggerUICallback('onBuildRenderSettingsTabs', cbContextDict, cbDataDict)
                cbDataDict = mayaUsdLib.triggerUICallback('onBuildAETemplate', cbContextDict, cbDataDict)
            except Exception as ex:
                # Do not let any of the callback failures affect our template.
                print('Failed triggerUICallback: %s' % ex)
        
        cmds.setUITemplate(ppt=True)


    def parseRenderSettingsAttributes(self):
        """Parse render settings attributes using the UsdRenderSettings node and property APIs."""

        self._properties = self.prim.GetProperties()
        # The root group for the "Common" tab
        rootDefGroup = AERenderSettingsLayout.Group("Common")
        # get the main sections for the render settings
        for property in self._properties:
            for group, properties in self.propertyGroups.items():
                if property.GetName() in properties:
                    if rootDefGroup.get(group) is None:
                        rootDefGroup.addItem(AERenderSettingsLayout.Group(group))
                    g = rootDefGroup.get(group)
                    g.addItem(property.GetName())
                    break            
        
        self._attributeLayout.items.append(rootDefGroup)
            
        # renderer settings tabs
        # create a new callback that 3rdparties can register on e.g 'onBuildRenderSettingsLayout'
        # that will be called to add additional tabs to the render settings section
        self.rendererSettingsTabs = {}

    def get(self):
        '''Computes the layout based on metadata ordering if an info list is present. If the list
           was computed in a different way, the attribute info list will be empty, and we return the
           computed attributeLayout unchanged.'''
        if not self._canCompute:
            return None

        self._attributeInfoList.sort()
        return self._attributeLayout
