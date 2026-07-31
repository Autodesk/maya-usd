#
# Copyright 2026 Autodesk
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
"""Does VP2 support camera-invisible-but-shadow-casting geometry at all?

No USD, no maya-usd -- plain Maya geometry only. This bounds the render pass
cameraVisibility question before any more effort goes into emulating it:

  * If TEST cube's shadow SURVIVES with primaryVisibility off, VP2 has the
    capability internally and the job is finding the MRenderItem surface for it.
  * If the shadow DIES too, VP2 cannot express it and cameraVisibility should
    fall back to a flag colour instead of a shadow-casting proxy.

Run inside Maya:

    import primaryVisibilityTest
    primaryVisibilityTest.show()

Two cubes on a ground plane. REF is never touched, so its shadow is a constant
reference -- if both shadows vanish, something in the setup broke rather than
the flag doing its job.
"""

from maya import cmds

WINDOW_NAME = 'primaryVisibilityTestWindow'

TEST_CUBE = 'testCube'
REF_CUBE = 'refCube'

# (attribute, expected effect) for the toggles below.
FLAGS = [
    ('primaryVisibility',
     'RESULT: VP2 ignores this. Cube and shadow both stay -- it is a batch-render '
     'stat, not a viewport flag.'),
    ('holdOut',
     'MATTE question, not cameraVisibility: a hold-out occludes but does not shade, '
     'so it is never camera-invisible. If VP2 honours it, matte could use it instead '
     'of a flag colour. Expect it to be ignored, like primaryVisibility.'),
    ('castsShadows',
     'Control: cube still visible, its shadow should vanish. '
     'Proves the shadow you are watching is really the test cube.'),
    ('visibility',
     'Control: cube and shadow should both vanish.'),
]


def buildScene():
    cmds.file(new=True, force=True)
    cmds.upAxis(axis='y')

    ground = cmds.polyPlane(name='ground', width=30, height=30,
                            subdivisionsX=1, subdivisionsY=1)[0]
    cmds.setAttr('{}.translateY'.format(ground), 0.0)

    for name, x in ((TEST_CUBE, -3.0), (REF_CUBE, 3.0)):
        cube = cmds.polyCube(name=name, width=2, height=2, depth=2)[0]
        cmds.setAttr('{}.translateY'.format(cube), 1.0)
        cmds.setAttr('{}.translateX'.format(cube), x)

    lightShape = cmds.directionalLight(intensity=1.4)
    lightTransform = cmds.listRelatives(lightShape, parent=True, fullPath=True)[0]
    cmds.setAttr('{}.rotateX'.format(lightTransform), -40.0)
    cmds.setAttr('{}.rotateY'.format(lightTransform), -30.0)
    cmds.setAttr('{}.useDepthMapShadows'.format(lightShape), 1)
    cmds.setAttr('{}.dmapResolution'.format(lightShape), 2048)

    for panel in cmds.getPanel(type='modelPanel') or []:
        try:
            cmds.modelEditor(panel, edit=True,
                             displayAppearance='smoothShaded',
                             displayLights='all',
                             shadows=True,
                             wireframeOnShaded=False)
        except RuntimeError:
            pass

    cmds.select(clear=True)
    cmds.viewFit(all=True)


def _shape(transform):
    return cmds.listRelatives(transform, shapes=True, fullPath=True)[0]


def _setIfPresent(shape, attr, value):
    """holdOut and friends are not on every shape type; skip what is missing."""
    if not cmds.attributeQuery(attr, node=shape, exists=True):
        return False
    # visibility defaults to 1 and holdOut to 0, so "reset" means the value that
    # leaves the cube behaving normally, which is 1 for all but holdOut.
    cmds.setAttr('{}.{}'.format(shape, attr), value)
    return True


def _resetValue(attr):
    return 0 if attr == 'holdOut' else 1


def _activeValue(attr):
    return 1 if attr == 'holdOut' else 0


def _reset(statusField):
    shape = _shape(TEST_CUBE)
    for attr, _ in FLAGS:
        _setIfPresent(shape, attr, _resetValue(attr))
    cmds.text(statusField, edit=True,
              label='All flags at defaults. Both cubes and both shadows should be visible.')
    cmds.refresh(force=True)


def _setFlagOff(attr, note, statusField):
    shape = _shape(TEST_CUBE)
    for other, _ in FLAGS:
        _setIfPresent(shape, other, _resetValue(other))

    if not _setIfPresent(shape, attr, _activeValue(attr)):
        cmds.text(statusField, edit=True,
                  label='{} does not exist on this shape -- nothing to test.'.format(attr))
        return

    cmds.text(statusField, edit=True, label='{} = {}  --  {}'.format(
        attr, _activeValue(attr), note))
    cmds.refresh(force=True)


def show():
    buildScene()

    if cmds.window(WINDOW_NAME, exists=True):
        cmds.deleteUI(WINDOW_NAME)

    cmds.window(WINDOW_NAME, title='primaryVisibility shadow test', widthHeight=(560, 240))
    cmds.columnLayout(adjustableColumn=True, rowSpacing=6, columnOffset=('both', 10))

    cmds.text(label='TEST cube is on the left, REF cube on the right (never changes).',
              align='left')
    cmds.separator(height=8, style='in')

    statusField = cmds.text(label='All flags ON.', align='left', wordWrap=True, height=40)

    cmds.button(label='Reset (all flags at defaults)',
                command=lambda *_: _reset(statusField))

    for attr, note in FLAGS:
        cmds.button(
            label='{} = {}  --  {}'.format(attr, _activeValue(attr), note),
            align='left',
            command=lambda *_, a=attr, n=note: _setFlagOff(a, n, statusField))

    cmds.showWindow(WINDOW_NAME)

    print('If you see no shadows at all with everything ON, the setup is wrong '
          'and no conclusion can be drawn.')


show()