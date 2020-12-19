import os
import unittest

import maya.cmds as cmds
import maya.OpenMaya as om
import maya.standalone as mayastandalone

import fixturesUtils

from pxr import Usd


class MayaUsdBindTransformExportTestCase(unittest.TestCase):
    MAYAUSD_PLUGIN_NAME = 'mayaUsdPlugin'

    @classmethod
    def setUpClass(cls):
        if not cmds.pluginInfo(cls.MAYAUSD_PLUGIN_NAME, q=True, loaded=True):
            cmds.loadPlugin('mayaUsdPlugin')

        cls.temp_dir = fixturesUtils.setUpClass(__file__)

    def setUp(self):
        testSceneFilePath = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__)))), "testSamples", 'bindTransforms', 'bindTransformsExport.ma')
        print testSceneFilePath
        om.MFileIO.open(testSceneFilePath, None, True)

    @classmethod
    def tearDownClass(cls):
        mayastandalone.uninitialize()

    def testBindTransformExport(self):
        temp_export_file = os.path.join(self.temp_dir, 'test_output.usda')
        cmds.select('a', r=True)
        cmds.mayaUSDExport(v=True, sl=True, f=temp_export_file, skl='auto', skn='auto', defaultMeshScheme='catmullClark')

        stage = Usd.Stage.Open(temp_export_file)
        prim = stage.GetPrimAtPath('/a/b/c')
        bindTransforms = prim.GetAttribute('bindTransforms').Get()
        self.assertNotEqual(bindTransforms, [[1, 0, 0, 0],
                                             [0, 1, 0, 0],
                                             [0, 0, 1, 0],
                                             [0, 0, 0, 1]])

if __name__ == '__main__':
    unittest.main()
