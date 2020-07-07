#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

import os
import tempfile
import ufe
import unittest

import maya.cmds as cmds

from mayaUsd import lib as mayaUsdLib
from mayaUsd.lib import GetPrim
from mayaUsd.lib import proxyAccessor as pa

from pxr import Usd, UsdGeom, Sdf, Tf, Vt

from maya.debug.emModeManager import emModeManager
from maya.plugin.evaluator.CacheEvaluatorManager import CacheEvaluatorManager, CACHE_STANDARD_MODE_EVAL

class NonCachingScope(object):
    '''
    Scope object responsible for setting up non cached mode and restoring default settings after
    '''
    def __enter__(self):
        '''Enter the scope, setting up the evaluator managers and initial states'''
        self.em_mgr = emModeManager()
        self.em_mgr.setMode('emp')
        self.em_mgr.setMode('-cache')

        return self

    def __init__(self, unit_test):
        '''Initialize everything to be empty - only use the "with" syntax with this object'''
        self.em_mgr = None
        self.unit_test = unit_test

    def __exit__(self,exit_type,value,traceback):
        '''Exit the scope, restoring all of the state information'''
        if self.em_mgr:
            self.em_mgr.restore_state()
            self.em_mgr = None

    def verifyScopeSetup(self):
        '''
        Meta-test to check that the scope was defined correctly
        :param unit_test: The test object from which this method was called
        '''
        self.unit_test.assertTrue( cmds.evaluationManager( mode=True, query=True )[0] == 'parallel' )
        if cmds.pluginInfo('cacheEvaluator', loaded=True, query=True):
            self.unit_test.assertFalse( cmds.evaluator( query=True, en=True, name='cache' ) )

    def checkValidFrames(self, expected_valid_frames, layers_mask = 0b01):
        return True

    def waitForCache(self, wait_time=5):
        return

    @staticmethod
    def is_caching_scope():
        '''
        Method to determine whether caching is on or off in this object's scope
        :return: False, since this is the non-caching scope
        '''
        return False

class CachingScope(object):
    '''
    Scope object responsible for setting up caching and restoring original setup after
    '''
    def __enter__(self):
        '''Enter the scope, setting up the evaluator managers and initial states'''
        self.em_mgr = emModeManager()
        self.em_mgr.setMode('emp')
        self.em_mgr.setMode('+cache')
        # Enable idle build to make sure we can rebuild the graph when waiting.
        self.em_mgr.idle_action = emModeManager.idle_action_build

        # Setup caching options
        self.cache_mgr = CacheEvaluatorManager()
        self.cache_mgr.save_state()
        self.cache_mgr.plugin_loaded = True
        self.cache_mgr.enabled = True
        self.cache_mgr.cache_mode = CACHE_STANDARD_MODE_EVAL
        self.cache_mgr.resource_guard = False
        self.cache_mgr.fill_mode = 'syncAsync'

        # Setup autokey options
        self.auto_key_state = cmds.autoKeyframe(q=True, state=True)
        self.auto_key_chars = cmds.autoKeyframe(q=True, characterOption=True)
        cmds.autoKeyframe(e=True, state=False)

        self.waitForCache()

        return self

    def __init__(self, unit_test):
        '''Initialize everything to be empty - only use the "with" syntax with this object'''
        self.em_mgr = None
        self.cache_mgr = None
        self.auto_key_state = None
        self.auto_key_chars = None
        self.unit_test = unit_test

    def __exit__(self,exit_type,value,traceback):
        '''Exit the scope, restoring all of the state information'''
        if self.cache_mgr:
            self.cache_mgr.restore_state()
        if self.em_mgr:
            self.em_mgr.restore_state()
        cmds.autoKeyframe(e=True, state=self.auto_key_state, characterOption=self.auto_key_chars)

    def verifyScopeSetup(self):
        '''
        Meta-test to check that the scope was defined correctly
        :param unit_test: The test object from which this method was called
        '''
        self.unit_test.assertTrue( cmds.evaluationManager( mode=True, query=True )[0] == 'parallel' )
        self.unit_test.assertTrue( cmds.pluginInfo('cacheEvaluator', loaded=True, query=True) )
        self.unit_test.assertTrue( cmds.evaluator( query=True, en=True, name='cache' ) )

    def checkValidFrames(self, expected_valid_frames, layers_mask = 0b01):
        '''
        :param unit_test: The test object from which this method was called
        :param expected_valid_frames: The list of frames the text expected to be cached
        :return: True if the cached frame list matches the expected frame list
        '''
        current_valid_frames = list(self.cache_mgr.get_valid_frames(layers_mask))
        if len(expected_valid_frames) == len(current_valid_frames):
            for current, expected in zip(current_valid_frames,expected_valid_frames):
                if current[0] != expected[0] or current[1] != expected[1]:
                    self.unit_test.fail( "{} != {} (current,expected)".format( current_valid_frames, expected_valid_frames) )
                    return False

            return True
        self.unit_test.fail( "{} != {} (current,expected)".format( current_valid_frames, expected_valid_frames) )
        return False

    def waitForCache(self, wait_time=5):
        '''
        Fill the cache in the background, waiting for a maximum time
        :param unit_test: The test object from which this method was called
        :param wait_time: Time the test is willing to wait for cache completion (in seconds)
        '''
        cmds.currentTime( cmds.currentTime(q=True) )
        cmds.currentTime( cmds.currentTime(q=True) )
        cache_is_ready = cmds.cacheEvaluator( waitForCache=wait_time )
        self.unit_test.assertTrue( cache_is_ready )

    @staticmethod
    def is_caching_scope():
        '''
        Method to determine whether caching is on or off in this object's scope
        :return: True, since this is the caching scope
        '''
        return True

def isPluginLoaded(pluginName):
    """
    Verifies that the given plugin is loaded
    Args:
        pluginName (str): The plugin name to verify
    Returns:
        True if the plugin is loaded. False if a plugin failed to load
    """
    return cmds.pluginInfo( pluginName, loaded=True, query=True)

def loadPlugin(pluginName):
    """
    Load all given plugins created or needed by maya-ufe-plugin
    Args:
        pluginName (str): The plugin name to load
    Returns:
        True if all plugins are loaded. False if a plugin failed to load
    """
    try:
        if not isPluginLoaded(pluginName):
            cmds.loadPlugin( pluginName, quiet = True )
        return True
    except:
        print(sys.exc_info()[1])
        print("Unable to load %s" % pluginName)
        return False

def isMayaUsdPluginLoaded():
    """
    Load plugins needed by tests.
    Returns:
        True if plugins loaded successfully. False if a plugin failed to load
    """
    # Load the mayaUsdPlugin first.
    if not loadPlugin("mayaUsdPlugin"):
        return False

    # Load the UFE support plugin, for ufeSelectCmd support.  If this plugin
    # isn't included in the distribution of Maya (e.g. Maya 2019 or 2020), use
    # fallback test plugin.
    if not (loadPlugin("ufeSupport") or loadPlugin("ufeTestCmdsPlugin")):
        return False

    # The renderSetup Python plugin registers a file new callback to Maya.  On
    # test application exit (in TbaseApp::cleanUp()), a file new is done and
    # thus the file new callback is invoked.  Unfortunately, this occurs after
    # the Python interpreter has been finalized, which causes a crash.  Since
    # renderSetup is not needed for mayaUsd tests, unload it.
    rs = 'renderSetup'
    if cmds.pluginInfo(rs, q=True, loaded=True):
        unloaded = cmds.unloadPlugin(rs)
        return (unloaded[0] == rs)

    return True

def makeUfePath(dagPath, sdfPath=None):
    """
    Make ufe item out of dag path and sdfpath
    """
    ufePath = ufe.PathString.path('{},{}'.format(dagPath,sdfPath) if sdfPath != None else '{}'.format(dagPath))
    ufeItem = ufe.Hierarchy.createItem(ufePath)
    return ufeItem

def selectUfeItems(selectItems):
    """
    Add given UFE item or list of items to a UFE global selection list
    """
    ufeSelectionList = ufe.Selection()
    
    realListToSelect = selectItems if type(selectItems) is list else [selectItems]
    for item in realListToSelect:
        ufeSelectionList.append(item)
    
    ufe.GlobalSelection.get().replaceWith(ufeSelectionList)

def createProxyAndStage():
    """
    Create in-memory stage
    """
    cmds.createNode('mayaUsdProxyShape', name='stageShape')

    shapeNode = cmds.ls(sl=True,l=True)[0]
    shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()
    
    cmds.select( clear=True )
    cmds.connectAttr('time1.outTime','{}.time'.format(shapeNode))

    return shapeNode,shapeStage

def createProxyFromFile(filePath):
    """
    Load stage from file
    """
    cmds.createNode('mayaUsdProxyShape', name='stageShape')

    shapeNode = cmds.ls(sl=True,l=True)[0]
    cmds.setAttr('{}.filePath'.format(shapeNode), filePath, type='string')
    
    shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()
    
    cmds.select( clear=True )
    cmds.connectAttr('time1.outTime','{}.time'.format(shapeNode))

    return shapeNode,shapeStage

def createAnimatedHierarchy(stage):
    """
    Create simple hierarchy in the stage:
    /ParentA
        /Sphere
        /Cube
    /ParenB
    
    Entire ParentA hierarchy will receive time samples on translate for time 1 and 100
    """
    parentA = "/ParentA"
    parentB = "/ParentB"
    childSphere = "/ParentA/Sphere"
    childCube = "/ParentA/Cube"
    
    parentPrimA = stage.DefinePrim(parentA, 'Xform')
    parentPrimB = stage.DefinePrim(parentB, 'Xform')
    childPrimSphere = stage.DefinePrim(childSphere, 'Sphere')
    childPrimCube = stage.DefinePrim(childCube, 'Cube')
    
    UsdGeom.XformCommonAPI(parentPrimA).SetRotate((0,0,0))
    UsdGeom.XformCommonAPI(parentPrimB).SetTranslate((1,10,0))
    
    time1 = Usd.TimeCode(1.)
    UsdGeom.XformCommonAPI(parentPrimA).SetTranslate((0,0,0),time1)
    UsdGeom.XformCommonAPI(childPrimSphere).SetTranslate((5,0,0),time1)
    UsdGeom.XformCommonAPI(childPrimCube).SetTranslate((0,0,5),time1)
    
    time2 = Usd.TimeCode(100.)
    UsdGeom.XformCommonAPI(parentPrimA).SetTranslate((0,5,0),time2)
    UsdGeom.XformCommonAPI(childPrimSphere).SetTranslate((-5,0,0),time2)
    UsdGeom.XformCommonAPI(childPrimCube).SetTranslate((0,0,-5),time2)

class MayaUsdProxyAccessorTestCase(unittest.TestCase):
    """
    Verify mayaUsd ProxyAccessor.
    """
    
    pluginsLoaded = False # Was mayaUsdPlugin loaded in this test class. Set in setUpClass.
    testDir = None # Folder where test files will be written. Set in setUpClass.
    testAnimatedHierarchyUsdFile = None # Usd file with content generated by createAnimatedHierarchy method
    cache_allFrames = [[1,120]] # Frames which have to be valid when entire cache is populated
    cache_empty = [] # Entire cache is invalid,
    
    @classmethod
    def setUpClass(cls):
        """
        Load mayaUsdPlugin on class initialization (before any test is executed)
        """
        if not cls.pluginsLoaded:
            cls.pluginsLoaded = isMayaUsdPluginLoaded()
        
        # Useful for debugging accessor
        #Tf.Debug.SetDebugSymbolsByName("USDMAYA_PROXYACCESSOR", 1)
        
        cls.testDir = os.path.join(os.path.abspath('.'),'TestMayaUsdProxyAccessor')
        tmpUsdFile = os.path.join(cls.testDir,'AnimatedHierarchy.usda')
        
        layer = Sdf.Layer.CreateAnonymous('TmpLayer')
        stage = Usd.Stage.Open(layer.identifier)
        createAnimatedHierarchy(stage)
        layer.Export(tmpUsdFile)
        
        cls.testAnimatedHierarchyUsdFile = tmpUsdFile

    @classmethod
    def tearDownClass(cls):
        """
        Cleanup after all tests run
        """
        cmds.file(new=True, force=True)

    def setUp(self):
        """
        Called initially to set up the maya test environment
        """
        self.assertTrue(self.pluginsLoaded)
    
    def assertVectorAlmostEqual(self, a, b, places=7):
        """
        Helper for vector almost equal assert
        """
        for va, vb in zip(a, b):
            self.assertAlmostEqual(va, vb, places)

    def assertMatrixAlmostEqual(self, ma, mb):
        for ra, rb in zip(ma, mb):
            for a, b in zip(ra, rb):
                self.assertAlmostEqual(a, b)

    def validatePlugsEqual(self, node, plugsWithResults):
        for plug, expected in plugsWithResults:
            result = cmds.getAttr('{}.{}'.format(node,plug))
            self.assertEqual(result, [expected])

    def validatePlugsAlmostEqual(self, node, plugsWithResults):
        for plug, expected in plugsWithResults:
            result = cmds.getAttr('{}.{}'.format(node,plug))
            self.assertAlmostEqual(result, [expected])

    def validateOutput(self,cachingScope):
        """
        Validate that accessor can output attributes and worldspace matrix from the stage
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Get UFE items
        ufeParentItemA = makeUfePath(nodeDagPath,'/ParentA')
        ufeChildItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        
        # Create accessor plugs
        worldMatrixPlugA = pa.getOrCreateAccessPlug(ufeParentItemA, '', Sdf.ValueTypeNames.Matrix4d )
        worldMatrixPlugSphere = pa.getOrCreateAccessPlug(ufeChildItemSphere, '', Sdf.ValueTypeNames.Matrix4d )
        translatePlugSphere = pa.getOrCreateAccessPlug(ufeChildItemSphere, usdAttrName='xformOp:translate' )
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Validate output accessor plugs
        cmds.currentTime(1)
        v0 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugA))
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = cmds.getAttr('{}.{}'.format(nodeDagPath,translatePlugSphere))
        self.assertEqual(v0, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0])
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        self.assertEqual(v2, [(5.0, 0.0, 0.0)])
        
        cmds.currentTime(100)
        v0 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugA))
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = cmds.getAttr('{}.{}'.format(nodeDagPath,translatePlugSphere))
        self.assertEqual(v0, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 5.0, 0.0, 1.0])
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -5.0, 5.0, 0.0, 1.0])
        self.assertEqual(v2, [(-5.0, 0.0, 0.0)])

    def validateOutputForInMemoryRootLayer(self,cachingScope):
        """
        Validate that accessor can output attributes and worldspace matrix from the stage.
        This helper method is creating stage with in-memory root layer.
        """
        nodeDagPath,stage = createProxyAndStage()
        createAnimatedHierarchy(stage)
        
        # Get UFE items
        ufeParentItemA = makeUfePath(nodeDagPath,'/ParentA')
        
        # Create accessor plugs
        worldMatrixPlugA = pa.getOrCreateAccessPlug(ufeParentItemA, '', Sdf.ValueTypeNames.Matrix4d )
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Validate output accessor plugs
        cmds.currentTime(1)
        v0 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugA))
        self.assertEqual(v0, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v0 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugA))
        self.assertEqual(v0, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 5.0, 0.0, 1.0])

    def validateTransformedOutput(self,cachingScope):
        """
        Validate that accessor will respect shape transform when writing world space outputs
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Animate proxy shape transform
        transform = cmds.listRelatives(nodeDagPath,parent=True)[0]
        cmds.setKeyframe( '{}.ry'.format(transform), time=1.0, value=0 )
        cmds.setKeyframe( '{}.ry'.format(transform), time=100.0, value=90 )
        
        # Get UFE items
        ufeItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        worldMatrixPlugSphere = pa.getOrCreateAccessPlug(ufeItemSphere, '', Sdf.ValueTypeNames.Matrix4d )
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Validate that DAG transformation is applied when reading world space matrix from USD
        cmds.currentTime(1)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        self.assertVectorAlmostEqual(v1, [0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 5.0, 5.0, 1.0])

    def validateInput(self, cachingScope):
        """
        Validate that accessor can write data to the stage and it's propagated correctly to:
        - output plugs
        - UFE interfaces
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Get UFE items
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        ufeItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        ufeItemCube = makeUfePath(nodeDagPath,'/ParentA/Cube')
        
        # Create accessor plugs
        translatePlugParent = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        rotatePlugParent = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:rotateXYZ')
        worldMatrixPlugSphere = pa.getOrCreateAccessPlug(ufeItemSphere, '', Sdf.ValueTypeNames.Matrix4d)
        
        # Instanciate interface object for UFE transform3d
        ufeTransform3dCube = ufe.Transform3d.transform3d(ufeItemCube)
        
        # Animate accessor plugs (they will become inputs to the stage owned by proxy)
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,translatePlugParent), time=1.0, value=0.0 )
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,translatePlugParent), time=100.0, value=10.0)
        
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,rotatePlugParent), time=1.0, value=0.0 )
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,rotatePlugParent), time=100.0, value=90.0)
        
        # Validate that input accessor plugs wrote the data to USD
        # Validate that output accessor plug has data driven by input accessor plug
        
        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        cmds.currentTime(1)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = ufeTransform3dCube.segmentInclusiveMatrix()
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        self.assertMatrixAlmostEqual(v2.matrix, [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 5.0, 1.0]])
        
        cmds.currentTime(100)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = ufeTransform3dCube.segmentInclusiveMatrix()
        self.assertVectorAlmostEqual(v1, [0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 10.0, 10.0, 15.0, 1.0])
        self.assertMatrixAlmostEqual(v2.matrix, [[0.0, 0.0, -1.0, 0.0], [0.0, 1.0, 0.0, 0.0], [1.0, 0.0, 0.0, 0.0], [5.0, 10.0, 10.0, 1.0]])

    def validateParentingDagObjectUnderUsdPrim(self, cachingScope):
        """
        Parent DagObject under UsdPrim
        """
        nodeDagPath,stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Animate proxy shape transform
        transform = cmds.listRelatives(nodeDagPath,parent=True)[0]
        cmds.setKeyframe( '{}.ry'.format(transform), time=1.0, value=0 )
        cmds.setKeyframe( '{}.ry'.format(transform), time=100.0, value=90 )
        
        # Create a locator to parent under /ParentA/Sphere
        # and apply offset on its translateY
        cmds.spaceLocator()
        childNodeDagPath = cmds.ls(sl=True,l=True)[0]
        cmds.setAttr('{}.ty'.format(childNodeDagPath), 3)
        
        # Get UFE items
        ufeItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        ufeItemChild = makeUfePath(childNodeDagPath)
        
        # Parent
        pa.parentItems([ufeItemChild],ufeItemSphere)
        
        # Validate
        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        cmds.currentTime(1)
        v1 = cmds.getAttr('{}.wm[0]'.format(childNodeDagPath))
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 3.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v1 = cmds.getAttr('{}.wm[0]'.format(childNodeDagPath))
        self.assertVectorAlmostEqual(v1, [0.0, 0.0, -1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 8.0, 5.0, 1.0])

    def validateSerialization(self,fileName, fileType):
        """
        Helper method for testing serialization.
        Test is a merge of DG connectivity and compute from testTransformedOutput, testInput and testParentingDagObjectUnderUsdPrim
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Create a locator and parent under /ParentA/Sphere
        # Apply offset on its translateY
        cmds.spaceLocator()
        locatorNodeDagPath = cmds.ls(sl=True,l=True)[0]
        cmds.setAttr('{}.ty'.format(locatorNodeDagPath), 3)
        
        # Get UFE items
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        ufeItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        ufeItemCube = makeUfePath(nodeDagPath,'/ParentA/Cube')
        ufeItemLocator = makeUfePath(locatorNodeDagPath)
        
        translatePlugParent = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        rotatePlugParent = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:rotateXYZ')
        worldMatrixPlugSphere = pa.getOrCreateAccessPlug(ufeItemSphere, '', Sdf.ValueTypeNames.Matrix4d)
        
        # Animate proxy shape transform
        transform = cmds.listRelatives(nodeDagPath,parent=True)[0]
        cmds.setKeyframe( '{}.ry'.format(transform), time=1.0, value=0 )
        cmds.setKeyframe( '{}.ry'.format(transform), time=100.0, value=90 )
        
        # Animate ParentA Translate and Rotate input plugs
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,translatePlugParent), time=1.0, value=0.0 )
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,translatePlugParent), time=100.0, value=10.0)
        
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,rotatePlugParent), time=1.0, value=0.0 )
        cmds.setKeyframe( '{}.{}'.format(nodeDagPath,rotatePlugParent), time=100.0, value=90.0)
        
        pa.parentItems([ufeItemLocator],ufeItemSphere)
        
        # Validate state before save
        cmds.currentTime(1)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = cmds.getAttr('{}.wm[0]'.format(locatorNodeDagPath)) # should be offset by 3 since we modified ty above
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        self.assertEqual(v2, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 3.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = cmds.getAttr('{}.wm[0]'.format(locatorNodeDagPath)) # should be offset by 3 since we modified ty above
        self.assertVectorAlmostEqual(v1, [-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 15.0, 10.0, -10.0, 1.0])
        self.assertVectorAlmostEqual(v2, [-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 15.0, 13.0, -10.0, 1.0])
        
        # Save and re-load the scene
        tmpMayaFile = os.path.join(self.testDir,fileName)
        cmds.file(rename=tmpMayaFile)
        cmds.file(save=True, type=fileType)
        cmds.file(new=True, force=True)
        cmds.file(tmpMayaFile, open=True, force=True)
        
        # Validate the saved scene
        cmds.currentTime(1)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = cmds.getAttr('{}.wm[0]'.format(locatorNodeDagPath)) # should be offset by 3 since we modified ty above
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        self.assertEqual(v2, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 3.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        v2 = cmds.getAttr('{}.wm[0]'.format(locatorNodeDagPath)) # should be offset by 3 since we modified ty above
        self.assertVectorAlmostEqual(v1, [-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 15.0, 10.0, -10.0, 1.0])
        self.assertVectorAlmostEqual(v2, [-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 15.0, 13.0, -10.0, 1.0])

    def validatePassivelyAffectedReset(self, cachingScope):
        """
        This helper method will validate that temporary manipulation is properly
        reset when no keys were dropped after manipulation.
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Get UFE item and select it
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        selectUfeItems(ufeItemParent)
        
        # Current limitation requires access plugs to be created before we start manipulating and keying
        translatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        rotatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:rotateXYZ')
        cmds.currentTime(1) # trigger compute to fill the data
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Manipulate using Maya and key (at frame 1)
        cmds.currentTime(1)
        cmds.move(0,-2,0, relative=True)
        cmds.rotate(0, 90, 0, relative=True, objectSpace=True, forceOrderXYZ=True)

        cachingScope.checkValidFrames(self.cache_empty)

        # Proxy accessor should pick up the manipulation and write it to output plugs
        # This is important to enable keying workflow
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])
        
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')
     
        # Double check that nothing changed after keying
        # Keying will create a new curve and connect the plug making
        # accessor plug an input now (since it has a connection)
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Manipulate again, but this time we don't key
        # Expected result is that after time change we should get back previously
        # keyed value (as per regular animation workflow in Maya)
        cmds.move(20,20,20, relative=True)
        cmds.rotate(45, 0, 0, relative=True, objectSpace=True, forceOrderXYZ=True)
        
        # Because we already have animation connected to accessor plugs,
        # above manipulation should only passively affect the data coming from connections.
        # Until we key, this should not invalidate the cache and values should be reset
        # with dirty propagation (caused by curve manipulation or times change)
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(20.0, 18.0, 20.0)), (rotatePlug, (45.0, 90.0, 0.0))])
        
        cmds.currentTime(50) # this time change should reset the value
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        # This reset shouldn't invalidate the cache
        cachingScope.checkValidFrames(self.cache_allFrames)

    def validateDagManipulation(self,cachingScope):
        """
        This helper method validate proper DAG invalidation during manipulation
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Animate proxy shape transform
        transform = cmds.listRelatives(nodeDagPath,parent=True)[0]
        
        # Get UFE items
        ufeItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        worldMatrixPlugSphere = pa.getOrCreateAccessPlug(ufeItemSphere, '', Sdf.ValueTypeNames.Matrix4d )
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Validate that DAG transformation is applied when reading world space matrix from USD
        cmds.currentTime(1)
        v0 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        self.assertEqual(v0, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v0 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        self.assertEqual(v0, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -5.0, 5.0, 0.0, 1.0])
        
        # Animate the proxy shape
        cmds.select(transform)
        
        cmds.currentTime(1)
        cmds.move(0,-2,0, relative=True)
        
        # Move should have invalidated entire cache since we moved static object
        cachingScope.checkValidFrames(self.cache_empty)
        
        cmds.setKeyframe('{}.t'.format(transform))
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        cmds.currentTime(50)
        cmds.move(-2,0,0, relative=True)
        
        # We already have one key dropped, so we shouldn't invalidate cache on this move
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        cmds.setKeyframe('{}.t'.format(transform))
        # We just added a new key, now cache should be invalid
        cachingScope.checkValidFrames(self.cache_empty)
    
    def validateKeyframeWithCommands(self, cachingScope):
        """
        This helper method will validate keying workflow (manipulation and setting keys to manipulated values)
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Get UFE item and select it
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        selectUfeItems(ufeItemParent)
        
        # Current limitation requires access plugs to be created before we start manipulating and keying
        translatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        rotatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:rotateXYZ')
        cmds.currentTime(1) # trigger compute to fill the data
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Manipulate using Maya and key (at frame 1)
        cmds.currentTime(1)
        cmds.move(0,-2,0, relative=True)
        cmds.rotate(0, 90, 0, relative=True, objectSpace=True, forceOrderXYZ=True)

        cachingScope.checkValidFrames(self.cache_empty)

        # Proxy accessor should pick up the manipulation and write it to output plugs
        # This is important to enable keying workflow
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])
        
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')
     
        # Double check that nothing changed after keying
        # Keying will create a new curve and connect the plug making
        # accessor plug an input now (since it has a connection)
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Manipulate and drop a key at frame 50
        cmds.currentTime(50)
        cmds.move(-20, -18, -20, relative=False)
        cmds.rotate(90, 0, 0, relative=False)
        
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')
        
        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Manipulate and drop a key at frame 100
        cmds.currentTime(100)
        cmds.move(10, -18, 10, relative=False)
        cmds.rotate(90, 45, 45, relative=False)
        
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')
        
        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Last pass over keys to validate animation
        cmds.currentTime(1)
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])
        
        cmds.currentTime(50)
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(-20.0, -18.0, -20.0)), (rotatePlug, (90.0, 00.0, 0.0))])

        cmds.currentTime(100)
        self.validatePlugsEqual(nodeDagPath,
            [(translatePlug,(10.0, -18.0, 10.0)), (rotatePlug, (90.0, 45.0, 45.0))])
            
    def validateKeyframeWithUFE(self, cachingScope):
        """
        This helper method will validate keying workflow (manipulation using UFE interface and setting keys to manipulated values)
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Get UFE item and select it
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        
        # Current limitation requires access plugs to be created before we start manipulating and keying
        translatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        rotatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:rotateXYZ')
        cmds.currentTime(1) # trigger compute to fill the data
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Manipulate using UFE and key (at frame 1)
        cmds.currentTime(1)

        ufeTransform3d = ufe.Transform3d.transform3d(ufeItemParent)
        v0 = ufeTransform3d.translation().vector
        v1 = ufeTransform3d.rotation().vector
        self.assertEqual(v0, [0.0, 0.0, 0.0])
        self.assertEqual(v1, [0.0, 0.0, 0.0])

        ufeTransform3d.translate(0.0, -2.0, 0.0)
        ufeTransform3d.rotate(0.0, 90.0, 0.0)

        # There is no key, so this should have invalidated the cache
        cachingScope.checkValidFrames(self.cache_empty)

        # Proxy accessor should pick up the manipulation and write it to output plugs
        # This is important to enable keying workflow
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')

        # Double check that nothing changed after keying
        # Keying will create a new curve and connect the plug making
        # accessor plug an input now (since it has a connection)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Manipulate and drop a key at frame 50
        cmds.currentTime(50)
        ufeTransform3d.translate(-20.0, -18.0, -20.0)
        ufeTransform3d.rotate(90.0, 0.0, 0.0)

        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')

        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Manipulate and drop a key at frame 100
        cmds.currentTime(100)
        ufeTransform3d.translate(-10.0, -18.0, -10.0)
        ufeTransform3d.rotate(90.0, 45.0, 45.0)

        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')

        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Last pass over keys to validate animation
        cmds.currentTime(1)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        cmds.currentTime(50)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(-20.0, -18.0, -20.0)), (rotatePlug, (90.0, 00.0, 0.0))])

        cmds.currentTime(100)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(10.0, -18.0, 10.0)), (rotatePlug, (90.0, 45.0, 45.0))])
    
    def validateKeyframeWithUSD(self, cachingScope):
        """
        This helper method will validate keying workflow (manipulation using USD interface and setting keys to manipulated values)
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Get UFE item and select it
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        
        path = Sdf.Path('/ParentA')
        usdPrim = stage.GetPrimAtPath(path)
        self.assertTrue(usdPrim)
        
        usdTranslateAttr = usdPrim.GetAttribute('xformOp:translate')
        self.assertTrue(usdTranslateAttr.IsDefined())
        usdRotateAttr = usdPrim.GetAttribute('xformOp:rotateXYZ')
        self.assertTrue(usdRotateAttr.IsDefined())
        
        # Current limitation requires access plugs to be created before we start manipulating and keying
        translatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        rotatePlug = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:rotateXYZ')
        cmds.currentTime(1) # trigger compute to fill the data
        
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        # Manipulate using USD and key (at frame 1)
        cmds.currentTime(1)
        timeCode = Usd.TimeCode.Default() #Usd.TimeCode(1.0) # We are picking candidates from default time!
        usdTranslateAttr.Set((0.0, -2.0, 0.0), timeCode)
        usdRotateAttr.Set((0.0, 90.0, 0.0), timeCode)

        # There is no key, so this should have invalidated the cache
        cachingScope.checkValidFrames(self.cache_empty)

        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')

        # Double check that nothing changed after keying
        # Keying will create a new curve and connect the plug making
        # accessor plug an input now (since it has a connection)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Manipulate and drop a key at frame 50
        cmds.currentTime(50)
        timeCode = Usd.TimeCode.Default() #Usd.TimeCode(50.0) # We are picking candidates from default time!
        usdTranslateAttr.Set((-20.0, -18.0, -20.0), timeCode)
        usdRotateAttr.Set((90.0, 0.0, 0.0), timeCode)

        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')

        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Manipulate and drop a key at frame 100
        cmds.currentTime(100)
        timeCode = Usd.TimeCode.Default() #Usd.TimeCode(100.0) # We are picking candidates from default time!
        usdTranslateAttr.Set((-10.0, -18.0, -10.0), timeCode)
        usdRotateAttr.Set((90.0, 45.0, 45.0), timeCode)

        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:translate')
        pa.keyframeAccessPlug(ufeItemParent, 'xformOp:rotateXYZ')

        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)

        # Last pass over keys to validate animation
        cmds.currentTime(1)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(0.0, -2.0, 0.0)), (rotatePlug, (0.0, 90.0, 0.0))])

        cmds.currentTime(50)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(-20.0, -18.0, -20.0)), (rotatePlug, (90.0, 00.0, 0.0))])

        cmds.currentTime(100)
        self.validatePlugsEqual(nodeDagPath,
           [(translatePlug,(10.0, -18.0, 10.0)), (rotatePlug, (90.0, 45.0, 45.0))])
    
    def validateDisconnect(self, cachingScope):
        """
        Validate that accessor clears temporary data after disconnecting the input
        """
        nodeDagPath, stage = createProxyFromFile(self.testAnimatedHierarchyUsdFile)
        
        # Animate accessor plugs (they will become inputs to the stage owned by proxy)
        cmds.spaceLocator()
        srcLocatorDagPath = cmds.ls(sl=True,l=True)[0]
        cmds.setKeyframe( '{}.tz'.format(srcLocatorDagPath), time=1.0, value=0.0 )
        cmds.setKeyframe( '{}.tz'.format(srcLocatorDagPath), time=100.0, value=10.0)
        
        # Get UFE items
        ufeItemParent = makeUfePath(nodeDagPath,'/ParentA')
        ufeItemSphere = makeUfePath(nodeDagPath,'/ParentA/Sphere')
        ufeItemSrcLocator = makeUfePath(srcLocatorDagPath)
        
        # Create accessor plugs
        translatePlugParent = pa.getOrCreateAccessPlug(ufeItemParent, usdAttrName='xformOp:translate')
        worldMatrixPlugSphere = pa.getOrCreateAccessPlug(ufeItemSphere, '', Sdf.ValueTypeNames.Matrix4d )
        
        pa.connectItems(ufeItemSrcLocator, ufeItemParent, [('translate','xformOp:translate')])
        
        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        cmds.currentTime(1)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        self.assertEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 5.0, 0.0, 0.0, 1.0])
        
        cmds.currentTime(100)
        v1 = cmds.getAttr('{}.{}'.format(nodeDagPath,worldMatrixPlugSphere))
        self.assertVectorAlmostEqual(v1, [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -5.0, 0.0, 10.0, 1.0])
        
        # Validate session layer
        dumpSessionLayer = stage.GetSessionLayer().ExportToString()
        # We will have many frames stored, but we only check last one (the rest is not really important for this test)
        self.assertIn("double3 xformOp:translate.timeSamples = {\n",dumpSessionLayer)
        self.assertIn("100: (0, 0, 10),\n",dumpSessionLayer)
            
        # Disconnect the attributes should clear temp opinions in session layers
        cmds.disconnectAttr('{}.t'.format(srcLocatorDagPath), '{}.{}'.format(nodeDagPath,translatePlugParent))
        
        cachingScope.checkValidFrames(self.cache_empty)
        cachingScope.waitForCache()
        cachingScope.checkValidFrames(self.cache_allFrames)
        
        dumpSessionLayer = stage.GetSessionLayer().ExportToString()
        self.assertNotIn("double3 xformOp:translate.timeSamples = {\n",dumpSessionLayer)
        
    ###################################################################################
    def testOutput_NoCaching(self):
        """
        Validate that accessor can output attributes and worldspace matrix from the stage
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateOutput(thisScope)

    def testOutput_Caching(self):
        """
        Validate that accessor can output attributes and worldspace matrix from the stage
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateOutput(thisScope)

    def testTransformedOutput_NoCaching(self):
        """
        Validate that accessor will respect shape transform when writing world space outputs.
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateTransformedOutput(thisScope)

    def testTransformedOutput_Caching(self):
        """
        Validate that accessor will respect shape transform when writing world space outputs.
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateTransformedOutput(thisScope)
    
    def testInput_NoCaching(self):
        """
        The that accessor can write data to the stage and it's propagated correctly to:
        - output plugs
        - UFE interfaces
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateInput(thisScope)
  
    def testInput_Caching(self):
        """
        The that accessor can write data to the stage and it's propagated correctly to:
        - output plugs
        - UFE interfaces
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateInput(thisScope)
        
    def testParentingDagObjectUnderUsdPrim_NoCaching(self):
        """
        Test parenting of dag object under usd prim.
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateParentingDagObjectUnderUsdPrim(thisScope)
 
    def testParentingDagObjectUnderUsdPrim_Caching(self):
        """
        Test parenting of dag object under usd prim.
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateParentingDagObjectUnderUsdPrim(thisScope)
 
    def testPassiveManipulation_NoCaching(self):
        """
        Test manipulation of accessor plug which has incoming curve connection.
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validatePassivelyAffectedReset(thisScope)
            
    def testPassiveManipulation_Caching(self):
        """
        Test manipulation of accessor plug which has incoming curve connection.
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validatePassivelyAffectedReset(thisScope)

    def testDagManipulation_NoCaching(self):
        """
        This helper method validate proper DAG invalidation during manipulation
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateDagManipulation(thisScope)
            
    def testDagManipulation_Caching(self):
        """
        This helper method validate proper DAG invalidation during manipulation
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateDagManipulation(thisScope)

    def testKeyframeWithCommands_NoCaching(self):
        """
        Test keying with cached playback disabled
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateKeyframeWithCommands(thisScope)
            
    def testKeyframeWithCommands_Caching(self):
        """
        Test keying with cached playback ENABLED
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateKeyframeWithCommands(thisScope)
            
    def testSerializationASCII_NoCache(self):
        """
        Test serialization in ascii maya file format
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateSerialization('testSerializationASCII.ma', 'mayaAscii')

    def testSerializationBinary_NoCache(self):
        """
        Test serialization in binary maya file format
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateSerialization('testSerializationBinary.mb', 'mayaBinary')
   
    def testOutputForInMemoryRootLayer_NoCache(self):
       """
       Validate that accessor can output attributes and worldspace matrix from the stage.
       This helper method is creating stage with in-memory root layer.
       Cached playback is disabled in this test.
       """
       cmds.file(new=True, force=True)
       with NonCachingScope(self) as thisScope:
           thisScope.verifyScopeSetup()
           self.validateOutputForInMemoryRootLayer(thisScope)
           
    def testOutputForInMemoryRootLayer_Cache(self):
        """
        Validate that accessor can output attributes and worldspace matrix from the stage.
        This helper method is creating stage with in-memory root layer.
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateOutputForInMemoryRootLayer(thisScope)
            
    @unittest.skip("Need to investigate why this test is not failing when run in Maya with GUI")
    def testKeyframeWithUFE_NoCaching(self):
        """
        This helper method will validate keying workflow (manipulation using UFE interface and setting keys to manipulated values)
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateKeyframeWithUFE(thisScope)

    @unittest.skip("Need to investigate why directly setting values with USD doesn't work. Could be a bug in the test")
    def testKeyframeWithUSD_NoCaching(self):
        """
        This helper method will validate keying workflow (manipulation using USD interface and setting keys to manipulated values)
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateKeyframeWithUSD(thisScope)

    def testDisconnect_NoCaching(self):
        """
        Validate that accessor clears temporary data after disconnecting the input
        Cached playback is disabled in this test.
        """
        cmds.file(new=True, force=True)
        with NonCachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateDisconnect(thisScope)

    def testDisconnect_Caching(self):
        """
        Validate that accessor clears temporary data after disconnecting the input
        Cached playback is ENABLED in this test.
        """
        cmds.file(new=True, force=True)
        with CachingScope(self) as thisScope:
            thisScope.verifyScopeSetup()
            self.validateDisconnect(thisScope)
