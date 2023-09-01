import maya.cmds as cmds
import fixturesUtils
import mtohUtils
from testUtils import PluginLoaded

# Test to ensure the C++ Testing Framework works as expected 

class TestCppFramework(mtohUtils.MayaHydraBaseTestCase):
    # MayaHydraBaseTestCase.setUpClass requirement.
    _file = __file__

    def test_Pass(self):
        self.setHdStormRenderer()
        with PluginLoaded('mayaHydraCppTests'):
            cmds.mayaHydraCppTest(f="CppFramework.pass")


    def test_Fail(self):
        self.setHdStormRenderer()
        with PluginLoaded('mayaHydraCppTests'):
            with self.assertRaises(RuntimeError):
                cmds.mayaHydraCppTest(f="CppFramework.fail")

if __name__ == '__main__':
    fixturesUtils.runTests(globals())