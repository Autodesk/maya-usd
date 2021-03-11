#!/usr/bin/env python

#
# Copyright 2019 Autodesk
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

import fixturesUtils
import mayaUtils
import ufeUtils
import usdUtils

from maya import cmds
from maya import standalone

import ufe

import os
import sys
import unittest


class MayaUFEPickWalkTesting(unittest.TestCase):
    '''
        These tests are to verify the hierarchy interface provided by UFE into multiple
        runtimes. It will be testing out Pickwalk on Maya.
        
        UFE Feature : Hierarchy
        Maya Feature : Pickwalk
        Action : Pickwalk [Up, Down, Left Right]
        Applied On Selection :
            - No selection - Given node as param
            - Single Selection [Maya, Non-Maya]
            - Multiple Selection [Mixed, Non-Mixed]
        Undo/Redo Test : Yes
        Expect Results To Test :
            - Maya Selection
            - UFE Selection
        Edge Cases :
            - Down on last item (Down again)
            - Up on top item (Up again)
            - Maya item walk to Non-Maya Item
            - Non-Maya item walk to Maya Item
            - Walk on non-supported UFE items
    '''
    
    pluginsLoaded = False

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

        if not cls.pluginsLoaded:
            cls.pluginsLoaded = mayaUtils.isMayaUsdPluginLoaded()
    
    @classmethod
    def tearDownClass(cls):
        # Clearing the undo stack with file new avoids mayapy crashing on
        # exit while cleaning out the undo stack, because the Python commands 
        # we use in this test are destroyed after the Python interpreter exits.
        cmds.file(new=True, force=True)

        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(self.pluginsLoaded)
        
        # Set up memento - [[mayaSelection, ufeSelection], ...]
        self.memento = []
        
        # Open top_layer.ma scene in testSamples
        mayaUtils.openTopLayerScene()
        
        # Create some extra Maya nodes
        cmds.polySphere()
        cmds.polyCube()
        
        # Clear selection to start off
        cmds.select(clear=True)
        
    def snapShotAndTest(self, expectedResults = None):
        ''' 
            Take a snapshot of the current selection in Maya and UFE.
            Test these selection on the expected results. If None are given,
            UFE and maya selections are equal.
            Args:
                expectedResults ([mayaSelection, ufeSelection]): 
                    Expected results to test on.
        '''
        mayaSelection = mayaUtils.getMayaSelectionList()
        ufeSelection = ufeUtils.getUfeGlobalSelectionList()
        self.memento.append((mayaSelection, ufeSelection))
        if expectedResults:
            self.assertEqual(self.memento[-1][0], expectedResults[0])
            self.assertEqual(self.memento[-1][1], expectedResults[1])
        else :
            self.assertEqual(self.memento[-1][0], self.memento[-1][1])
            
    def rewindMemento(self):
        ''' 
            Undo through all items in memento. Make sure that the current
            selection match their respective registered selection in memento.
        '''
        for m in reversed(self.memento[:-1]):
            cmds.undo()
            self.assertEqual(m[0], mayaUtils.getMayaSelectionList())
            self.assertEqual(m[1], ufeUtils.getUfeGlobalSelectionList())
            
    def fforwardMemento(self):
        ''' 
            Redo through all items in memento. Make sure that the current
            selection match their respective registered selection in memento.
        '''
        # Test initial
        self.assertEqual(self.memento[0][0], mayaUtils.getMayaSelectionList())
        self.assertEqual(self.memento[0][1], ufeUtils.getUfeGlobalSelectionList())
        # Skip first
        for m in self.memento[1:]:
            cmds.redo()
            self.assertEqual(m[0], mayaUtils.getMayaSelectionList())
            self.assertEqual(m[1], ufeUtils.getUfeGlobalSelectionList())
    
    def test_pickWalk(self):
        '''
            This test will be verify the use of Maya's pickWalk into Maya objects and Usd objects.  
        '''
        # Initial state check up 
        self.snapShotAndTest()
        
        # No Selection
        cmds.pickWalk("pSphere1", d="down")
        self.snapShotAndTest()
                
        # Single Selection - Maya Item
        cmds.select("pCube1")
        self.snapShotAndTest()
        
        cmds.pickWalk(d="down")
        self.snapShotAndTest()
        
        # Edge Case: Maya item to UFE Item
        cmds.select("proxyShape1")
        self.snapShotAndTest()
        cmds.pickWalk(d="down") # /Room_set
        self.snapShotAndTest(([], ['Room_set']))
        
        # Edge Case: UFE Item to Maya
        cmds.pickWalk(d="up") # /proxyShape1
        self.snapShotAndTest()
        
        # Pickwalk all the way down in USD - Pickwalk down again
        for usdItem in ['Room_set', 'Props', 'Ball_1', 'mesh']:
            cmds.pickWalk(d="down") # Room_set / Props / Ball_1 / mesh
            self.snapShotAndTest(([], [usdItem]))
            
        # Edge Case : Down on last item (Down again)
        cmds.pickWalk(d="down")
        self.snapShotAndTest(([], ['mesh']))
        
        # Edge Case : Up on top item (Up again)
        cmds.select("transform1")
        self.snapShotAndTest()
        cmds.pickWalk(d="up") # transform1
        self.snapShotAndTest() # World is not selectable in maya
        
        
        # Pickwalk on unsupported UFE items
        cmds.select("pCube1.e[6]")
        if mayaUtils.mayaMajorVersion() <= 2020:
            self.snapShotAndTest((["pCube1.e[6]"], ["pCubeShape1"]))
            self.assertTrue(next(iter(ufe.GlobalSelection.get())).isProperty())
        else:
            self.snapShotAndTest((["pCube1.e[6]"], []))
            self.assertTrue(ufe.GlobalSelection.get().empty())
        
        # TODO: HS 2019 : test fails.  MAYA-101373
#        cmds.pickWalk(type="edgeloop", d="right")
#        import pdb; pdb.set_trace()
#        self.snapShotAndTest((["pCube1.e[10]"], ["pCubeShape1"]))
#        self.assertTrue(next(iter(ufe.GlobalSelection.get())).isProperty()) # pCubeShape1 in ufe selection is a property
        
        
        # Pickwalk on Non-Mixed Maya items is already tested in Maya regression tests
        # Pickwalk on Non-Mixed USD items
        ufeUtils.selectPath(ufe.Path([ \
                               mayaUtils.createUfePathSegment("|transform1|proxyShape1"), \
                               usdUtils.createUfePathSegment("/Room_set/Props/Ball_1") \
                                ]), True)
        self.snapShotAndTest(([], ['Ball_1']))
        
        
        ufeUtils.selectPath(ufe.Path([ \
                               mayaUtils.createUfePathSegment("|transform1|proxyShape1"), \
                               usdUtils.createUfePathSegment("/Room_set/Props/Ball_2") \
                                ]))
        self.snapShotAndTest(([], ['Ball_1', 'Ball_2']))
        
        cmds.pickWalk(d="down")
        self.snapShotAndTest(([], ['mesh', 'mesh']))
        
        # Pickwalk on mixed items
        ufeUtils.selectPath(ufe.Path([ \
                               mayaUtils.createUfePathSegment("|transform1|proxyShape1"), \
                               usdUtils.createUfePathSegment("/Room_set/Props/Ball_1") \
                                ]), True)
        self.snapShotAndTest(([], ['Ball_1']))
        
        ufeUtils.selectPath(ufe.Path( mayaUtils.createUfePathSegment("|pCube1")))
        self.snapShotAndTest((['pCube1'], ['Ball_1', 'pCube1']))
        
        # Selection on Maya items from UFE does not work
        cmds.pickWalk(d="down")
        self.snapShotAndTest((['pCubeShape1'], ['mesh', 'pCubeShape1']))
        
        # Test Complete - Undo and redo everything
        ''' MAYA-92081 : Undoing cmds.select does not restore the UFE selection. 
            the undo(rewindMemento) would fail. Disabling this until defect is fixed
        self.rewindMemento()
        self.fforwardMemento()
        '''


if __name__ == '__main__':
    unittest.main(verbosity=2)
