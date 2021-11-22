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

import json
import os
import tempfile
import shutil
import unittest

import AL

import pxr
from pxr import Usd, UsdUtils

from maya import cmds
from maya.api import OpenMaya as om2


class TestProxyShapeGetUsdPrimFromMayaPath(unittest.TestCase):
    """Test cases for static function: AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath"""

    def __init__(self, *args, **kwargs):
        super(TestProxyShapeGetUsdPrimFromMayaPath, self).__init__(*args, **kwargs)

        self._stage = None
        self._sphere = None
        self._proxyName = None

    def setUp(self):
        """Export some sphere geometry as .usda, and import into a new Maya scene."""

        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

        _tmpfile = tempfile.NamedTemporaryFile(delete=True, suffix=".usda")
        _tmpfile.close()

        # Ensure sphere geometry exists
        self._sphere = cmds.polySphere(constructionHistory=False, name="sphere")[0]
        cmds.select(self._sphere)

        # Export, new scene, import
        cmds.file(_tmpfile.name, exportSelected=True, force=True, type="AL usdmaya export")
        cmds.file(force=True, new=True)
        self._proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=_tmpfile.name)[0]

        # Ensure proxy exists
        self.assertIsNotNone(self._proxyName)

        # Store stage
        proxy = AL.usdmaya.ProxyShape.getByName(self._proxyName)
        self._stage = proxy.getUsdStage()

        os.remove(_tmpfile.name)

    def tearDown(self):
        """Unload plugin, new Maya scene, reset class member variables."""

        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

        self._stage = None
        self._sphere = None
        self._proxyName = None

    def test_getUsdPrimFromMayaPath_success(self):
        """Find prim from Maya dag path successfully."""

        # Select prim, Maya node(s) created, query path
        cmds.AL_usdmaya_ProxyShapeSelect(proxy=self._proxyName, primPath="/{}".format(self._sphere))
        prim = AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath(self._sphere)
        if os.getenv('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE'):
            self.assertTrue(prim.IsValid())

    def test_getUsdPrimFromMayaPath_invalidPrim(self):
        """Dag paths msut exist and be associated with a prim to return a valid prim."""

        # Dag path of sphere is unselected, doesn't exist
        cmds.select(clear=True)
        prim = AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath(self._sphere)
        self.assertFalse(prim.IsValid())

        # 'foo' dag path doesn't exist in Maya
        prim = AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath("foo")
        self.assertFalse(prim.IsValid())

        # Query node unassociated with prim
        cube = cmds.polyCube(constructionHistory=False, name="foo")[0]
        self.assertTrue(cmds.objExists(cube))
        prim = AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath(cube)
        self.assertFalse(prim.IsValid())

    def test_getUsdPrimFromMayaPath_duplicateNames(self):
        """Test short name queries return an invalid prim if duplicates exist in the Maya scene."""

        # Create a transform with the same name as sphere
        cmds.createNode("transform", name=self._sphere)

        # Select prim, Maya node(s) created
        cmds.AL_usdmaya_ProxyShapeSelect(proxy=self._proxyName, primPath="/{}".format(self._sphere))

        # Two nodes exist in Maya named "sphere", short name queries return with invalid prim
        _sphereShortName = self._sphere
        prim = AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath(_sphereShortName)
        self.assertFalse(prim.IsValid())

        if os.getenv('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE'):
            # Query long name? Success!
            _sphereLongName = cmds.ls(self._sphere, type="AL_usdmaya_Transform")[0]
            prim = AL.usdmaya.ProxyShape.getUsdPrimFromMayaPath(_sphereLongName)
            self.assertTrue(prim.IsValid())
        else:
            print('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE enabled, ignoring VP2 render delegate check')

class TestProxyShapeGetMayaPathFromUsdPrim(unittest.TestCase):
    """Test cases for member function: AL.usdmaya.ProxyShape().getMayaPathFromUsdPrim"""

    class MayaUsdTestData:
        """Container that holds links to test data."""
        poly = None       # Short name path to Maya node, and name of prim in Usd stage
        proxyName = None  # AL_usdmaya_ProxyShape Maya node name
        proxy = None      # AL_usdmaya_ProxyShape instance
        stage = None      # Imported Usd stage that hosts prim
        prim = None       # Usd prim in stage

    def __init__(self, *args, **kwargs):
        super(TestProxyShapeGetMayaPathFromUsdPrim, self).__init__(*args, **kwargs)

        self._stageA = self.MayaUsdTestData()
        self._stageB = self.MayaUsdTestData()

    def setUp(self):
        """A few steps take place here:
        1. New scene in Maya, ensure plugin is loaded
        2. Create a polySphere and polyCube, export these using to separate Usd stages
        3. Import both stages together into a new Maya scene
        """

        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

        stageA_file = tempfile.NamedTemporaryFile(delete=True, suffix=".usda")
        stageB_file = tempfile.NamedTemporaryFile(delete=True, suffix=".usda")
        stageA_file.close()
        stageB_file.close()

        cube = cmds.polyCube(constructionHistory=False, name="cube")[0]
        sphere = cmds.polySphere(constructionHistory=False, name="cube")[0]

        cmds.select(cube, replace=True)
        cmds.file(stageA_file.name, exportSelected=True, force=True, type="AL usdmaya export")

        cmds.select(sphere, replace=True)
        cmds.file(stageB_file.name, exportSelected=True, force=True, type="AL usdmaya export")

        self._stageA.poly = cube
        self._stageB.poly = sphere

        cmds.file(force=True, new=True)

        self._stageA.proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=stageA_file.name)[0]
        self._stageB.proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=stageB_file.name)[0]

        self._stageA.proxy = AL.usdmaya.ProxyShape.getByName(self._stageA.proxyName)
        self._stageB.proxy = AL.usdmaya.ProxyShape.getByName(self._stageB.proxyName)

        self._stageA.stage = self._stageA.proxy.getUsdStage()
        self._stageB.stage = self._stageB.proxy.getUsdStage()

        self._stageA.prim = self._stageA.stage.GetPrimAtPath("/{}".format(self._stageA.poly))
        self._stageB.prim = self._stageB.stage.GetPrimAtPath("/{}".format(self._stageB.poly))

        os.remove(stageA_file.name)
        os.remove(stageB_file.name)

    def tearDown(self):
        """New Maya scene, unload plugin, reset data."""

        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

        self._stageA = self.MayaUsdTestData()
        self._stageB = self.MayaUsdTestData()

    def test_getMayaPathFromUsdPrim_success(self):
        """Maya scenes can contain multiple proxies. Query each proxy and test they return the correct Maya nodes."""

        if os.getenv('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE'):
            # These are dynamic prims, so select them to bring into Maya
            cmds.select(clear=True)
            cmds.AL_usdmaya_ProxyShapeSelect(self._stageA.proxyName, replace=True, primPath=str(self._stageA.prim.GetPath()))
            cmds.AL_usdmaya_ProxyShapeSelect(self._stageB.proxyName, append=True, primPath=str(self._stageB.prim.GetPath()))

            # Created Maya node names will match the originals
            self.assertTrue(cmds.objExists(self._stageA.poly))
            self.assertTrue(cmds.objExists(self._stageB.poly))

            # Fetch the Maya node paths that represent each stage's prims
            resultA = self._stageA.proxy.getMayaPathFromUsdPrim(self._stageA.prim)
            resultB = self._stageB.proxy.getMayaPathFromUsdPrim(self._stageB.prim)

            # Expand to long names
            expectedA = cmds.ls(self._stageA.poly, long=True)[0]
            expectedB = cmds.ls(self._stageB.poly, long=True)[0]

            # The paths should match!
            self.assertEqual(resultA, expectedA)
            self.assertEqual(resultB, expectedB)
        else:
            print('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE enabled, ignoring VP2 render delegate check')

    def test_getMayaPathFromUsdPrim_failure(self):
        """Query a proxy with an invalid prim."""

        result = self._stageA.proxy.getMayaPathFromUsdPrim(pxr.Usd.Prim())
        expected = ""

        self.assertEqual(result, expected)

    def test_getMayaPathFromUsdPrim_unselected(self):
        """Query a proxy with a valid prim who's Maya node has not been translated (unselected)."""

        result = self._stageA.proxy.getMayaPathFromUsdPrim(self._stageA.prim)
        expected = ""

        self.assertEqual(result, expected)

    def test_getMayaPathFromUsdPrim_primStageMismatch(self):
        """Query a proxy with a prim from a different Usd stage."""

        result = self._stageA.proxy.getMayaPathFromUsdPrim(self._stageB.prim)
        expected = ""

        self.assertEqual(result, expected)

    def test_getMayaPathFromUsdPrim_staticImport(self):
        """Force import a prim."""

        cmds.AL_usdmaya_ProxyShapeImportPrimPathAsMaya(self._stageA.proxyName, primPath=str(self._stageA.prim.GetPath()))
        result = self._stageA.proxy.getMayaPathFromUsdPrim(self._stageA.prim)
        expected = cmds.ls(self._stageA.poly, long=True)[0]

        if os.getenv('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE'):
            self.assertEqual(result, expected)

    @unittest.skip("Not working")
    def test_getMayaPathFromUsdPrim_reopenImport(self):
        """Saving and reopening a Maya scene with dynamic translated prims should work."""

        # Save
        _file = tempfile.NamedTemporaryFile(delete=False, suffix=".ma")
        with _file:
            cmds.file(rename=_file.name)
            cmds.file(save=True, force=True)
            self.assertFalse(cmds.ls(assemblies=True))
        _file.close()

        # Re-open
        cmds.file(_file.name, open=True, force=True)

        # Refresh data, stage wrapper is out of date
        _stageC = self.MayaUsdTestData()
        _stageC.poly = self._stageA.poly
        _stageC.proxyName = self._stageA.proxyName
        _stageC.proxy = AL.usdmaya.ProxyShape.getByName(_stageC.proxyName)
        _stageC.stage = _stageC.proxy.getUsdStage()
        _stageC.prim = _stageC.stage.GetPrimAtPath("/{}".format(_stageC.poly))

        # Test
        if os.getenv('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE'):
            cmds.AL_usdmaya_ProxyShapeSelect(_stageC.proxyName, replace=True, primPath=str(_stageC.prim.GetPath()))
            self.assertTrue(cmds.objExists(_stageC.poly))
            result = _stageC.proxy.getMayaPathFromUsdPrim(_stageC.prim)
            expected = cmds.ls(_stageC.poly, long=True)[0]
            self.assertEqual(result, expected)
        else:
            print('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE enabled, ignoring VP2 render delegate check')

        # Cleanup
        os.remove(_file.name)

    @unittest.skip("Not working")
    def test_getMayaPathFromUsdPrim_reopenStaticImport(self):
        """Saving and reopening a Maya scene with static translated prims should work."""

        cmds.AL_usdmaya_ProxyShapeImportPrimPathAsMaya(self._stageA.proxyName, primPath=str(self._stageA.prim.GetPath()))

        # Save
        _file = tempfile.NamedTemporaryFile(delete=False, suffix=".ma")
        with _file:
            cmds.file(rename=_file.name)
            cmds.file(save=True, force=True)
        _file.close()

        # Re-open
        cmds.file(_file.name, open=True, force=True)

        # Refresh data, stage wrapper is out of date
        _stageC = self.MayaUsdTestData()
        _stageC.poly = self._stageA.poly
        _stageC.proxyName = self._stageA.proxyName
        _stageC.proxy = AL.usdmaya.ProxyShape.getByName(_stageC.proxyName)
        _stageC.stage = _stageC.proxy.getUsdStage()
        _stageC.prim = _stageC.stage.GetPrimAtPath("/{}".format(_stageC.poly))

        # Test
        if os.getenv('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE'):
            cmds.AL_usdmaya_ProxyShapeSelect(_stageC.proxyName, replace=True, primPath=str(_stageC.prim.GetPath()))
            self.assertTrue(cmds.objExists(_stageC.poly))
            result = _stageC.proxy.getMayaPathFromUsdPrim(_stageC.prim)
            expected = cmds.ls(_stageC.poly, long=True)[0]
            self.assertEqual(result, expected)
        else:
            print('MAYAUSD_DISABLE_VP2_RENDER_DELEGATE enabled, ignoring VP2 render delegate check')

        # Cleanup
        os.remove(_file.name)


class TestProxyShapeAnonymousLayer(unittest.TestCase):
    workingDir = None
    fileName = 'foo.ma'
    SC = pxr.UsdUtils.StageCache.Get()

    def setUp(self):
        unittest.TestCase.setUp(self)
        self.workingDir = tempfile.mkdtemp()
        cmds.file(new=True, save=False, force=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

        with pxr.Usd.StageCacheContext(self.SC):
            stage = pxr.Usd.Stage.CreateInMemory()
            with pxr.Usd.EditContext(stage, pxr.Usd.EditTarget(stage.GetRootLayer())):
                pxr.UsdGeom.Xform.Define(stage, '/root/GEO/sphere_hrc').AddTranslateOp().Set((0.0, 1.0, 0.0))
                pxr.UsdGeom.Sphere.Define(stage, '/root/GEO/sphere_hrc/sphere').GetRadiusAttr().Set(5.0)

        cmds.AL_usdmaya_ProxyShapeImport(
            stageId=self.SC.GetId(stage).ToLongInt(),
            name='anonymousShape'
        )
        cmds.file(rename=self.serialisationPath())
        cmds.file(type='mayaAscii')
        cmds.file(save=True)
        cmds.file(new=True, save=False, force=True)
        self.SC.Clear()

    def tearDown(self):
        if os.path.isdir(self.workingDir):
            shutil.rmtree(self.workingDir)

        self.SC.Clear()
        unittest.TestCase.tearDown(self)

    def serialisationPath(self):
        return os.path.join(self.workingDir, self.fileName)

    def test_anonymousLayerRoundTrip(self):
        cmds.file(self.serialisationPath(), open=True, save=False, force=True)
        proxyShape = AL.usdmaya.ProxyShape.getByName('anonymousShape')
        self.assertIsNotNone(proxyShape)
        stage = proxyShape.getUsdStage()
        self.assertIsInstance(stage, pxr.Usd.Stage)

        hrc = stage.GetPrimAtPath('/root/GEO/sphere_hrc')
        self.assertTrue(hrc.IsValid())
        self.assertTrue(hrc.IsDefined())
        self.assertTrue(hrc.IsA(pxr.UsdGeom.Xform))
        hrcXformableApi = pxr.UsdGeom.XformCommonAPI(hrc)
        t, r, s, p, _ = hrcXformableApi.GetXformVectors(pxr.Usd.TimeCode.Default())
        self.assertEqual(tuple(t), (0.0, 1.0, 0.0))
        self.assertEqual(tuple(r), (0.0, 0.0, 0.0))
        self.assertEqual(tuple(s), (1.0, 1.0, 1.0))
        self.assertEqual(tuple(p), (0.0, 0.0, 0.0))

        sphere = stage.GetPrimAtPath('/root/GEO/sphere_hrc/sphere')
        self.assertTrue(sphere.IsValid())
        self.assertTrue(sphere.IsDefined())
        self.assertTrue(sphere.IsA(pxr.UsdGeom.Sphere))
        self.assertEqual(pxr.UsdGeom.Sphere(sphere).GetRadiusAttr().Get(), 5.0)


class TestProxyShapeVariantFallbacks(unittest.TestCase):
    def setUp(self):
        cmds.file(new=True, force=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)

    def tearDown(self):
        pxr.UsdUtils.StageCache.Get().Clear()
        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

    def _prepSessionLayer(self, customVariantFallbacks):
        defaultGlobalVariantFallbacks = pxr.Usd.Stage.GetGlobalVariantFallbacks()
        pxr.Usd.Stage.SetGlobalVariantFallbacks(customVariantFallbacks)
        usdFile = '../test_data/variant_fallbacks.usda'

        stage = pxr.Usd.Stage.Open(usdFile)
        stageCacheId = pxr.UsdUtils.StageCache.Get().Insert(stage)
        sessionLayer = stage.GetSessionLayer()
        sessionLayer.customLayerData = {'variant_fallbacks': json.dumps(customVariantFallbacks)}
        # restore default
        pxr.Usd.Stage.SetGlobalVariantFallbacks(defaultGlobalVariantFallbacks)

        # create the proxy node with stage cache id
        proxyName = cmds.AL_usdmaya_ProxyShapeImport(stageId=stageCacheId.ToLongInt())[0]
        proxyShape = AL.usdmaya.ProxyShape.getByName(proxyName)
        return proxyShape, proxyName

    def test_fromSessionLayer(self):
        # this will create a C++ type of: std::vector<VtValue>
        custom = {'geo': ['sphere']}
        proxyShape, proxyName = self._prepSessionLayer(custom)

        # the "sphere" variant prim should be loaded
        prim = proxyShape.getUsdStage().GetPrimAtPath('/root/GEO/sphere1/sphereShape1')
        self.assertTrue(prim.IsValid())

        # the custom variant fallback should be saved to the attribute
        savedAttrValue = cmds.getAttr(proxyName + '.variantFallbacks')
        self.assertEqual(savedAttrValue, json.dumps(custom))


if __name__ == "__main__":

    tests = [unittest.TestLoader().loadTestsFromTestCase(TestProxyShapeGetUsdPrimFromMayaPath),
             unittest.TestLoader().loadTestsFromTestCase(TestProxyShapeGetMayaPathFromUsdPrim),
             unittest.TestLoader().loadTestsFromTestCase(TestProxyShapeAnonymousLayer),
             unittest.TestLoader().loadTestsFromTestCase(TestProxyShapeVariantFallbacks),
              ]
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    cmds.quit(exitCode=(exitCode))
