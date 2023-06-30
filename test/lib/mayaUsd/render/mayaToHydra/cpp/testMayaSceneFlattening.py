import maya.cmds as cmds
import fixturesUtils
import mtohUtils
from testUtils import PluginLoaded

class TestMayaSceneFlattening(mtohUtils.MayaHydraBaseTestCase):
    # MayaHydraBaseTestCase.setUpClass requirement.
    _file = __file__

    def setupParentChildScene(self):
        self.setHdStormRenderer()
        cmds.polySphere(name="parentSphere")
        cmds.polyCube(name="childCube")
        cmds.parent("childCube", "parentSphere")
        cmds.move(1, 2, 3, "childCube", relative=True)
        cmds.move(-6, -4, -2, "parentSphere", relative=True)
        cmds.refresh()

    def test_ChildHasFlattenedTransform(self):
        self.setupParentChildScene()
        with PluginLoaded('mayaHydraCppTests'):
            cmds.mayaHydraCppTest(f="MayaSceneFlattening.childHasFlattenedTransform")

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
