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
    _isDecimal = re.compile("^[0-9]+$")

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
                print("createGroup", groupName, items)
                if isinstance(items, dict):
                    for name in items.keys():
                        _createGroup(name, items[name])
                else:
                    for attr in items:
                        print("createGroupItem", attr)
                        if self.template.attrS.hasAttribute(attr):
                            self.template.addControls([attr])

        # Create the callback context/data (empty).
        cbContext = {
            'ufe_path_string' : ufe.PathString.string(self.item.path()),
        }
        cbContextDict = Vt._ReturnDictionary(cbContext)
        cbDataDict = Vt._ReturnDictionary({})

        
        print("cbDataDict", cbDataDict)
        cmds.setUITemplate('attributeEditorTemplate', pst=True)        
        with ufeAeTemplate.Layout(self, "", collapse=False):
            # cmds.editorTemplate(beginNoOptimize=True)

            cmds.editorTemplate(beginTabLayout=True)

            # loop over the cbDataDict and create a tab for each
            # for tab in cbDataDict.keys():
            #     print("createGroupRoot", tab, cbDataDict[tab])
            #     _createGroup(tab, cbDataDict[tab])

            # Trigger the callback which will give other plugins the opportunity
            # to add controls to our AE template.
            try:
                cbDataDict = mayaUsdLib.triggerUICallback('onBuildRenderSettingsTabs', cbContextDict, cbDataDict)
            except Exception as ex:
                # Do not let any of the callback failures affect our template.
                print('Failed triggerUICallback: %s' % ex)
            # cmds.tabLayout(tabbar, edit=True, tabLabel=tabs)

            # # create a tab for each renderer
            # cmds.editorTemplate(beginChildTab='Arnold')
            # # ...do work here...
            # # cmds.button(label='Tab 1')     
            # with ufeAeTemplate.Layout(self, "Default", collapse=False):

            #     dummybutton = DummyButtonControl(self.item, self.prim, self.template)
            #     self.template.defineCustom(dummybutton)

            # with ufeAeTemplate.Layout(self, "System Settings", collapse=False):

            #     dummybutton = DummyButtonControl(self.item, self.prim, self.template)
            #     self.template.defineCustom(dummybutton)

            # cmds.editorTemplate(endChildTab=True)

            # cmds.editorTemplate(beginChildTab='Foo')
            # # ...do work here...
            # # cmds.button(label='Tab 1')

            # dummybutton = DummyButtonControl(self.item, self.prim, self.template)
            # self.template.defineCustom(dummybutton)


            # cmds.editorTemplate(endChildTab=True)
            # # cmds.editorTemplate(endTab=True)
            # # cmds.editorTemplate(endTabLayout=True)
            # # tab1 = cmds.rowColumnLayout(numberOfColumns=2)
            # # with LayoutManager(tab1):

            cmds.editorTemplate(endTabLayout=True)


            # self.template.runRenderSettingsCallback()
            # with ufeAeTemplate.Layout(self, "BAA", collapse=False):
            #     cmds.button(label='Button')

            # # frame layout
            # frame = cmds.frameLayout(collapsable=False, label='FOO', collapse=False, backgroundShade=True)
            # with LayoutManager(frame):
            #     column = cmds.columnLayout(adjustableColumn=True)
            #     with LayoutManager(column):
            #         cmds.button(label='Tab 1')
            #         # create the tabs
            #         # cmds.setParent(column)
            #         # tabbar = cmds.tabLayout(innerMarginHeight=5, parent=column)

            #         tab1 = cmds.columnLayout(adjustableColumn=True)
            #         with LayoutManager(tab1):
            #             cmds.button(label='Tab 1')

                    # tab2 = cmds.rowColumnLayout(numberOfColumns=2)
                    # with LayoutManager(tab2):
                    #     cmds.button(label='Tab 2')


                    # cmds.tabLayout(tabbar, edit=True, tabLabel=((tab1, 'Tab 1'), (tab2, 'Tab 2')))

                    # form = cmds.formLayout()
                    # with LayoutManager(form):
                    #     tabbar = cmds.tabLayout(innerMarginHeight=5)
                    #     cmds.formLayout( form, edit=True, attachForm=((tabbar , 'top', 0), (tabbar , 'left', 0), (tabbar , 'bottom', 0), (tabbar , 'right', 0)) )


                    #     cbDataDict = self.template.runRenderSettingsCallback()
                    #     cbDataDict['FOO'] = None

                    #     # loop over the rendererSettingsTabs and create a tab for each
                    #     tabs = []
                    #     for tab, controlLayout in cbDataDict.items():
                    #         print("createTabLayout", tab)
                    #         if controlLayout:
                    #             if isinstance(controlLayout, list):
                    #                 controlLayout = controlLayout[0]
                    #             tabs.append((controlLayout, tab))
                    #         else:
                    #             # create a blank tab
                    #             blankControlLayout = cmds.rowColumnLayout(numberOfColumns=2)
                    #             with LayoutManager(blankControlLayout):
                    #                 cmds.button(label='Apply Schema')
                    #                 # with ufeAeTemplate.Layout(self, "Apply Schema", collapse=False):
                    #                 #     dummybutton = DummyButtonControl(self.item, self.prim, self.template)
                    #                 #     self.template.defineCustom(dummybutton)
                            
                    #             tabs.append((blankControlLayout, tab))
                    #     cmds.setParent('..')
                        
                    #     cmds.tabLayout(tabbar, edit=True, tabLabel=tabs)
                        
            # columLayout

            # form = cmds.rowColumnLayout(numberOfColumns=2)
            # with LayoutManager(form):
            #     cmds.button(label='Apply Arnold Schema')

            # tabsLayoutControl = RenderSettingsTabLayout(self.item, self.prim, self.template)
            # create = lambda *args : tabsLayoutControl.onCreate(args)
            # replace = lambda *args : tabsLayoutControl.onReplace(args)
            # cmds.editorTemplate([], callCustom=[create, replace], debugMode=True)

            # self.template.defineCustom(tabsLayoutControl)
            # cmds.editorTemplate(endNoOptimize=True)

        with ufeAeTemplate.Layout(self, "After tabs", collapse=False):

            dummybutton = DummyButtonControl(self.item, self.prim, self.template)
            self.template.defineCustom(dummybutton)
        
        cmds.setUITemplate(ppt=True)


    def parseRenderSettingsAttributes(self):
        """Parse render settings attributes using the UsdRenderSettings node and property APIs."""

        self._properties = self.prim.GetProperties()
        rootDefLabel = mayaUsdUfe.prettifyName(self.prim.GetTypeName().replace('Render', ''))
        groups = {rootDefLabel: None, 'Base': None, 'Extra Attributes': None}
        # get the main sections for the render settings
        for property in self._properties:
            print("parseRenderSettingsAttributes", property.GetName())
            found = False
            for group, properties in self.propertyGroups.items():
                if property.GetName() in properties:
                    if groups.get(group) is None:
                        groups[group] = AERenderSettingsLayout.Group(group)
                    groups[group].items.append(property.GetName())
                    found = True
                    break
            
            # if not found:
            #     if groups['Extra Attributes'] is None:
            #         groups['Extra Attributes'] = AERenderSettingsLayout.Group('Extra Attributes')
            #     groups['Extra Attributes'].items.append(property.GetName())
        
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
        # folderIndex = {(): self._attributeLayout}

        # for attributeInfo in self._attributeInfoList:
        #     groups = tuple()
        #     # Add the attribute to the group
        #     folderIndex[groups].items.append(attributeInfo.name)
        return self._attributeLayout
