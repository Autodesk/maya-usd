"""
Tests for the 'AL_USDMaya_ProxyShape' proxyAccessor features.
"""
import os
import unittest

from pxr import Usd, UsdUtils

import ufe
from maya import cmds
from mayaUsd.lib import proxyAccessor as pa


IS_UFE_2 = int(cmds.about(majorVersion=True)) >= 2021

CUBE_OFFSET_STAGE = """#usda 1.0
    (
        defaultPrim = "pCube1"
        upAxis = "Y"
    )

    def Cube "pCube1" (
        kind = "component"
    )
    {
        matrix4d xformOp:transform = ((1,0,0,0),(0,1,0,0),(0,0,1,0),(0, 5, 10,1))
        uniform token[] xformOpOrder = ["xformOp:transform"]
    }
"""


@unittest.skipIf(
    not IS_UFE_2, "Proxy Accessor only available for ufe 2.0, skipping test"
)
class TestProxyAccessor(unittest.TestCase):
    """Test cases AL_USDMaya_ProxyShape proxyAccessor features"""

    app = "maya"

    def setUp(self):
        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

    def tearDown(self):
        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

    def test_connectAttributes(self):
        """Confirm connecting mayaDag attributes can drive usd attributes.

        """
        # arrange ------------------------------------------------------
        stage = self._createInMemoryStageFromString(CUBE_OFFSET_STAGE)
        proxyShapeDagPath = self._createProxyShapeFromStage(stage)

        cmds.spaceLocator()
        locatorDagPath = cmds.ls(sl=True, l=True)[0]

        usdUfeItem = pa.createUfeSceneItem(proxyShapeDagPath, "/pCube1")
        matrixPlug = pa.getOrCreateAccessPlug(
            usdUfeItem, usdAttrName="xformOp:transform"
        )

        # confirm transform has offset
        ufeCube = ufe.Transform3d.transform3d(usdUfeItem)
        expected = [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 5.0, 10.0, 1.0],
        ]
        self.assertEqual(ufeCube.segmentInclusiveMatrix().matrix, expected)

        # act ----------------------------------------------------------
        cmds.connectAttr(
            "{}.worldMatrix[0]".format(locatorDagPath),
            "{}.{}".format(proxyShapeDagPath, matrixPlug),
        )

        # assert -------------------------------------------------------
        # Confirm transform now matches locator at origin
        expected = [
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
        self.assertEqual(ufeCube.segmentInclusiveMatrix().matrix, expected)

    def _createInMemoryStageFromString(self, stageString):
        """Helper function to create stage from string.
        """
        stage = Usd.Stage.CreateInMemory()
        stage.GetRootLayer().ExportToString()
        stage.GetRootLayer().ImportFromString(stageString)
        return stage

    def _createProxyShapeFromStage(self, stage):
        """Creates a AL_usdmaya_ProxyShape node from a stage
        """
        # get the singleton stage cache
        stageCache = UsdUtils.StageCache.Get()
        if not stageCache.Contains(stage):
            stageCache.Insert(stage)

        # get the stage id
        stageId = stageCache.GetId(stage)

        proxyName = cmds.AL_usdmaya_ProxyShapeImport(
            stageId=stageId.ToLongInt(), name="testShape"
        )[0]

        return cmds.ls(proxyName, l=True)[0]


if __name__ == "__main__":

    tests = [
        unittest.TestLoader().loadTestsFromTestCase(TestProxyAccessor),
    ]
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    # Note: cmds.quit() does not return exit code as expected, we use Python builtin function instead
    os._exit(exitCode)
