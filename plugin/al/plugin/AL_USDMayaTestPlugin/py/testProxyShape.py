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
import shutil
import unittest

import AL

import pxr
from pxr import Usd, UsdUtils

from maya import cmds
from maya.api import OpenMaya as om2

import fixturesUtils


class TestProxyShapeAnonymousLayer(unittest.TestCase):
    workingDir = None
    fileName = 'foo.ma'
    SC = pxr.UsdUtils.StageCache.Get()

    @classmethod
    def setUpClass(cls):
        # Setup for test output
        fixturesUtils.setUpClass(__file__, loadPlugin=False)

    @classmethod
    def tearDownClass(cls):
        fixturesUtils.tearDownClass(unloadPlugin=False)

    def setUp(self):
        unittest.TestCase.setUp(self)
        self.workingDir = os.path.abspath(type(self).__name__)
        os.mkdir(self.workingDir)
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
    fixturesUtils.runTests(globals(), verbosity=2)
