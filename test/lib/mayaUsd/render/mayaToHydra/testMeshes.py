import maya.cmds as cmds
import maya.mel as mel

import fixturesUtils
import mtohUtils
import mayaUtils
from testUtils import PluginLoaded

import unittest

class TestMeshes(mtohUtils.MtohTestCase):
    # MtohTestCase.setUpClass requirement.
    _file = __file__

    def matchingRprims(self, rprims, matching):
        return len([rprim for rprim in rprims if matching in rprim])

    @unittest.skipUnless(mayaUtils.hydraFixLevel() > 0, "Requires Data Server render item lifescope fix.")
    def test_sweepMesh(self):
        self.setHdStormRenderer()
        with PluginLoaded('sweep'):
            mel.eval("performSweepMesh 0")
            cmds.refresh()

            # There should be a single rprim from the sweep shape.
            rprims = self.getIndex()
            self.assertEqual(1, self.matchingRprims(rprims, 'sweepShape'))

            # Change the scale profile.
            cmds.setAttr("sweepMeshCreator1.scaleProfileX", 2)
            cmds.refresh()

            # Should still be a single rprim from the sweep shape.
            rprims = self.getIndex()
            self.assertEqual(1, self.matchingRprims(rprims, 'sweepShape'))

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
