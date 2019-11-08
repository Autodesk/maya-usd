import os
import tempfile
import unittest

from maya import cmds
from AL.usdmaya import ProxyShape

class TestLayerManagerSerialisation(unittest.TestCase):
    """Test cases for layer manager serialisation and deserialisation"""

    app = 'maya'
    def setUp(self):
        """Export some sphere geometry as .usda, and import into a new Maya scene."""

        cmds.file(force=True, new=True)
        cmds.loadPlugin("AL_USDMayaPlugin", quiet=True)
        self.assertTrue(cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True))

        with tempfile.NamedTemporaryFile(delete=False, suffix=".usda") as _tmpfile:
            self._usdaFile = _tmpfile.name
            # Ensure sphere geometry exists
            self._sphere = cmds.polySphere(constructionHistory=False, name="sphere")[0]
            cmds.select(self._sphere)

            # Export, new scene, import
            cmds.file(self._usdaFile, exportSelected=True, force=True, type="AL usdmaya export")
            cmds.file(force=True, new=True)
            self._proxyName = cmds.AL_usdmaya_ProxyShapeImport(file=self._usdaFile)[0]

        # Ensure proxy exists
        self.assertIsNotNone(self._proxyName)

        # Store stage
        proxy = ProxyShape.getByName(self._proxyName)
        self._stage = proxy.getUsdStage()

    def tearDown(self):
        """Unload plugin, new Maya scene, reset class member variables."""

        cmds.file(force=True, new=True)
        cmds.unloadPlugin("AL_USDMayaPlugin", force=True)

        self._stage = None
        self._sphere = None
        self._proxyName = None
        if os.path.isfile(self._usdaFile):
            os.remove(self._usdaFile)

    def test_editTargetSerialisation(self):
        """ As long as we set an editTarget and it ends updirty, it should be serialised
            when we save Maya scene, and it should be reloadable by SdfLayer::Reload()
        """
        self.assertTrue(self._stage)
        self._stage.SetEditTarget(self._stage.GetRootLayer())
        newPrimPath = "/ChangeInRoot"
        self._stage.DefinePrim(newPrimPath, "xform")
        self._stage.SetEditTarget(self._stage.GetSessionLayer())
        with tempfile.NamedTemporaryFile(delete=True, suffix=".ma") as _tmpMayafile:
            cmds.file(rename=_tmpMayafile.name)
            cmds.file(save=True, force=True)
            cmds.file(new=True, force=True)
            cmds.file(_tmpMayafile.name, open=True)
            self.assertIsNotNone(self._proxyName)
            ps = ProxyShape.getByName(self._proxyName)
            self.assertTrue(ps)
            stage = ps.getUsdStage()
            self.assertTrue(stage)
            self.assertTrue(stage.GetPrimAtPath(newPrimPath))
            self.assertTrue(stage.GetRootLayer().Reload())
            self.assertFalse(stage.GetPrimAtPath(newPrimPath))

    def test_sessionLayerSerialisation(self):
        """ A clean session layer should not be serialised on Maya scene save, nor we get
            any complain form usdMaya on Maya scene reopen.
            A dirty session layer should be serialised correctly on Maya scene save, and 
            we should get it deserialised on Maya scene reopen. We should also be able to
            reload on session layer to clear it.
        """
        self.assertTrue(self._stage)
        self._stage.SetEditTarget(self._stage.GetSessionLayer())
        # Save the scene with clean session layer:
        with tempfile.NamedTemporaryFile(delete=True, suffix=".ma") as _tmpMayafile:
            cmds.file(rename=_tmpMayafile.name)
            cmds.file(save=True, force=True)
            cmds.file(new=True, force=True)
            cmds.file(_tmpMayafile.name, open=True)
            self.assertFalse(cmds.getAttr('%s.sessionLayerName' % self._proxyName))

        ps = ProxyShape.getByName(self._proxyName)
        self.assertTrue(ps)
        stage = ps.getUsdStage()
        # Make session layer dirty and save the scene:
        stage.SetEditTarget(stage.GetSessionLayer())
        newPrimPath = "/ChangeInSession"
        stage.DefinePrim(newPrimPath, "xform")
        with tempfile.NamedTemporaryFile(delete=True, suffix=".ma") as _tmpMayafile:
            cmds.file(rename=_tmpMayafile.name)
            cmds.file(save=True, force=True)
            cmds.file(new=True, force=True)
            cmds.file(_tmpMayafile.name, open=True)
            self.assertTrue(cmds.getAttr('%s.sessionLayerName' % self._proxyName))

            ps = ProxyShape.getByName(self._proxyName)
            self.assertTrue(ps)
            stage = ps.getUsdStage()
            self.assertTrue(stage)
            self.assertTrue(stage.GetPrimAtPath(newPrimPath))
            self.assertTrue(stage.GetSessionLayer().Reload())
            self.assertFalse(stage.GetPrimAtPath(newPrimPath))



if __name__ == "__main__":

    tests = [unittest.TestLoader().loadTestsFromTestCase(TestLayerManagerSerialisation),]
    results = [unittest.TextTestRunner(verbosity=2).run(test) for test in tests]
    exitCode = int(not all([result.wasSuccessful() for result in results]))
    cmds.quit(exitCode=(exitCode))