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
import json
import unittest

import fixturesUtils
import maya.cmds as cmds


class OldLayerEditorParityTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__, initializeStandalone=False, loadPlugin=False)
        cmds.loadPlugin('mayaUsdPlugin')
        plugin = 'mayaUsdOldLayerEditorTests'
        if not cmds.pluginInfo(plugin, q=True, loaded=True):
            cmds.loadPlugin(plugin)

    def test_old_layer_editor_parity(self):
        raw     = cmds.mayaUsd_runLayerEditorTests()
        results = json.loads(raw)
        failures = [r for r in results if not r['passed']]

        if failures:
            msg = '\n\n'.join(
                '{name}:\n{message}'.format(**r) for r in failures
            )
            self.fail('{n} test(s) failed:\n\n{msg}'.format(
                n=len(failures), msg=msg))


if __name__ == '__main__':
    fixturesUtils.runTests(globals())
