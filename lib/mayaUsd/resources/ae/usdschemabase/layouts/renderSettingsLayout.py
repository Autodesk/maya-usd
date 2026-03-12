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
import maya.mel as mel
import maya.cmds as cmds
import mayaUsd.lib as mayaUsdLib
import mayaUsd.ufe as mayaUsdUfe
import maya.internal.common.ufe_ae.template as ufeAeTemplate
from maya.common.ui import LayoutManager, ParentManager

from pxr import Vt

class DummyButtonControl(object):
    def __init__(self, item, prim, template):
        super(DummyButtonControl, self).__init__()
        self.item = item
        self.prim = prim
        self.template = template

    def onCreate(self, *args):
        # cmds.setUITemplate('attributeEditorTemplate', pst=True)
        rl = cmds.rowLayout(nc=1)
        with LayoutManager(rl):
            cmds.button(label='Apply Arnold Schema')
        
        # cmds.setUITemplate(ppt=True)

    def onReplace(self, *args):
        pass

class RenderSettingsTabLayout(object):

    def __init__(self, item, prim, template):
        super(RenderSettingsTabLayout, self).__init__()
        self.item = item
        self.prim = prim
        self.typeName = prim.GetTypeName()
        self.template = template # template to use to add controls to

    def onCreate(self, *args):
        cmds.setUITemplate('attributeEditorTemplate', pst=True)
        
        form = cmds.formLayout()
        with LayoutManager(form):
            tabbar = cmds.tabLayout(innerMarginHeight=5)
            cmds.formLayout( form, edit=True, attachForm=((tabbar , 'top', 0), (tabbar , 'left', 0), (tabbar , 'bottom', 0), (tabbar , 'right', 0)) )

            cbDataDict = self.template.runRenderSettingsCallback()

            # loop over the rendererSettingsTabs and create a tab for each
            tabs = []
            for tab, controlLayout in cbDataDict.items():
                print("createTabLayout", tab)
                # if controlLayout:
                #     if isinstance(controlLayout, list):
                #         controlLayout = controlLayout[0]
                #     tabs.append((controlLayout, tab))
                # else:
                # create a blank tab
                blankControlLayout = cmds.rowColumnLayout(numberOfColumns=2)
                with LayoutManager(blankControlLayout):
                    cmds.button(label='Apply Schema')
                    # with ufeAeTemplate.Layout(self, "Apply Schema", collapse=False):
                    #     dummybutton = DummyButtonControl(self.item, self.prim, self.template)
                    #     self.template.defineCustom(dummybutton)
                
                tabs.append((blankControlLayout, tab))

            if len(tabs) == 0:
                # create a blank tab
                blankControlLayout = cmds.rowColumnLayout(numberOfColumns=2)
                cmds.setParent('..')
                tabs.append((blankControlLayout, 'Empty'))
            
            print("tabs", tabs)
            cmds.tabLayout(tabbar, edit=True, tabLabel=tabs)
            
        
        cmds.setUITemplate(ppt=True)

    def onReplace(self, *args):
        pass



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
                'instantaneousShutter',
                'materialBindingPurposes',
                'products',
                'renderingColorSpace'
            ],
            'Base': [
                'resolution',
                'pixelAspectRatio',
                'aspectRatioConformPolicy',
                'dataWindowNDC',
                'disableMotionBlur',
                'disableDepthOfField',
                'camera',
            ],
        }

        self.parseRenderSettingsAttributes()

    def createRendererTabsLayout(self):
        """Create the tab layout for the render settings section"""

        def _createGroup(groupName, items):
            
            with ufeAeTemplate.Layout(self, groupName, collapse=False):
                if isinstance(items, dict):
                    for name in items.keys():
                        _createGroup(name, items[name])
                else:
                    for attr in items:
                        if self.template.attrs.hasAttribute(attr):
                            self.template.addControls([attr])

        # Create the callback context/data (empty).
        cbContext = {
            'ufe_path_string' : ufe.PathString.string(self.item.path()),
        }
        cbContextDict = Vt._ReturnDictionary(cbContext)
        cbDataDict = Vt._ReturnDictionary({})

        cmds.setUITemplate('attributeEditorTemplate', pst=True)    
        with ufeAeTemplate.Layout(self, "", collapse=False):
            # cmds.editorTemplate(beginNoOptimize=True)

            if cmds.about(apiVersion=True) >= 20270100:
                cmds.editorTemplate(beginTabLayout=True)
            
            for item in self._attributeLayout.items:
                if isinstance(item, AERenderSettingsLayout.Group):
                    _createGroup(item.name, item.items)
                else:
                    if self.template.attrs.attribute(item):
                        self.template.addControls([item])
            # Trigger the callback which will give other plugins the opportunity
            # to add controls to our AE template.
            try:
                cbDataDict = mayaUsdLib.triggerUICallback('onBuildRenderSettingsTabs', cbContextDict, cbDataDict)
            except Exception as ex:
                # Do not let any of the callback failures affect our template.
                print('Failed triggerUICallback: %s' % ex)

            if cmds.about(apiVersion=True) >= 20270100:
                cmds.editorTemplate(endTabLayout=True)

        with ufeAeTemplate.Layout(self, "After tabs", collapse=False):

            dummybutton = DummyButtonControl(self.item, self.prim, self.template)
            self.template.defineCustom(dummybutton)
        
        cmds.setUITemplate(ppt=True)


    def parseRenderSettingsAttributes(self):
        """Parse render settings attributes using the UsdRenderSettings node and property APIs."""

        self._properties = self.prim.GetProperties()
        rootDefGroup = AERenderSettingsLayout.Group(mayaUsdUfe.prettifyName(self.prim.GetTypeName().replace('Render', '')))
        groups = {rootDefGroup: None, 'Base': None, 'Extra Attributes': None}
        # get the main sections for the render settings
        for property in self._properties:
            for group, properties in self.propertyGroups.items():
                if property.GetName() in properties:
                    if groups.get(group) is None:
                        groups[group] = AERenderSettingsLayout.Group(group)
                    groups[group].items.append(property.GetName())
                    break            
        
        for group in groups.keys():
            if groups[group]:
                self._attributeLayout.items.append(groups[group])
            
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
