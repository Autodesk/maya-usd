import maya.cmds as cmds

import fixturesUtils
import mtohUtils
import mayaUtils
from testUtils import PluginLoaded

import unittest
import sys

def checkForMayaUsd():
    try:
        cmds.loadPlugin('mayaUsdPlugin')
    except:
        return False
    
    return True
    
class TestStage(mtohUtils.MayaHydraBaseTestCase):
    # MayaHydraBaseTestCase.setUpClass requirement.
    _file = __file__
          
    @unittest.skipUnless(checkForMayaUsd(), "Requires Maya USD Plugin.")
    def test_addPrim(self):
        import mayaUsd_createStageWithNewLayer
        import mayaUsd.lib

        self.setHdStormRenderer()
        # empty scene and expect zero prims
        rprims = self.getIndex()
        self.assertEqual(0, len(rprims))
        
        # start with a maya sphere
        sphere = cmds.polySphere()
        cmds.refresh()
        rprims = self.getIndex()
        # we expect non-zero rprims(two here)
        rprimsBefore = len(rprims)
        self.assertGreater(rprimsBefore, 0)
        
        # add an empty USD Stage.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        
        # duplicate the sphere into Stage as USD data
        self.assertTrue(mayaUsd.lib.PrimUpdaterManager.duplicate(cmds.ls(sphere[0], long=True)[0], psPathStr))
        cmds.refresh()
        rprims = self.getIndex()
        rprimsAfter = len(rprims)
        self.assertGreater(rprimsAfter, rprimsBefore)
        cmds.delete(sphere[0])
        
        cmds.refresh()
        rprims = self.getIndex()
        # we expect a non-zero rprim count(one here)
        rprimsAfter = len(rprims)
        self.assertGreater(rprimsAfter, 0)
            

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
