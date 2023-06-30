import maya.cmds as cmds

import fixturesUtils
import mtohUtils
import unittest

from testUtils import PluginLoaded

class TestMayaUsdUfeItems(mtohUtils.MayaHydraBaseTestCase):
    # MayaHydraBaseTestCase.setUpClass requirement.
    _file = __file__

    def setupUsdStage(self):
        import mayaUsd
        import usdUtils
        import ufe
        
        self.setHdStormRenderer()

        proxyShapeUfePathString, proxyShapeUfePath, proxyShapeUfeItem = usdUtils.createSimpleStage()
        stage = mayaUsd.lib.GetPrim(proxyShapeUfePathString).GetStage()

        # Create a light prim
        rectLightName = "myRectLight"
        rectLightPrim = stage.DefinePrim('/' + rectLightName, 'RectLight')
        rectLightUfePathString = proxyShapeUfePathString + ',/' + rectLightName
        rectLightUfePath = ufe.PathString.path(rectLightUfePathString)
        rectLightUfeItem = ufe.Hierarchy.createItem(rectLightUfePath)

        # Create a geometry prim
        sphereName = "mySphere"
        sphere = cmds.polySphere(name=sphereName)
        mayaUsd.lib.PrimUpdaterManager.duplicate(cmds.ls(sphere, long=True)[0], proxyShapeUfePathString)

        cmds.refresh()

    @unittest.skipUnless(mtohUtils.checkForMayaUsdPlugin(), "Requires Maya USD Plugin.")
    def test_SkipMayaUsdUfeItems(self):
        self.setupUsdStage()
        with PluginLoaded('mayaHydraCppTests'):
            cmds.mayaHydraCppTest(f="MayaUsdUfeItems.skipUsdUfeItems")

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
