#!/usr/bin/env python

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

import unittest

import mayaUtils
import fixturesUtils
import mayaUsd.lib
import mayaUsd.ufe
import tempfile
from maya import cmds

import os

def _isCCAvailable():
    if 'MAYA_MODULE_PATH' not in os.environ:
        return False
    for pp in os.environ['MAYA_MODULE_PATH'].split(os.path.pathsep):
        if os.path.exists(pp) and 'usd_component_creator.mod' in os.listdir(pp):
            return True
    return False

_CC_AVAILABLE = _isCCAvailable()

class LoadComponentCreatorPluginTestCase(unittest.TestCase):
    """
    Verify that the USD component creator plugin can be loaded.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def testPluginLoadable(self):
        if not _CC_AVAILABLE:
            forceTest = os.environ.get('MAYAUSD_FORCE_CC_TEST', '0') == '1'
            if forceTest:
                self.fail('USD component creator plugin was not found but MAYAUSD_FORCE_CC_TEST is set. '
                          'MAYA_MODULE_PATH={}'.format(os.environ.get('MAYA_MODULE_PATH', '')))
            else:
                self.skipTest('Could not find the USD component creator plugin')

        self.assertTrue(mayaUtils.loadPlugin('mayaUsdPlugin'))
        self.assertTrue(mayaUtils.loadPlugin('usd_component_creator'))


class DuplicateToComponentTestCase(unittest.TestCase):
    """
    Test duplicating Maya nodes to an Adsk USD Component stage via the
    'Duplicate as USD' command.
    """

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, initializeStandalone=False)

    def setUp(self):
        if not _CC_AVAILABLE:
            self.skipTest('Could not find the USD component creator plugin')
        mayaUtils.loadPlugin('mayaUsdPlugin')
        mayaUtils.loadPlugin('usd_component_creator')
        cmds.file(new=True, force=True)

    def _createComponent(self):
        """Creates a new Adsk USD Component and a proxy shape pointing to it.

        Returns (proxyShapePath, ComponentDescription).
        """
        import AdskUsdComponentCreator
        
        tempDir = tempfile.mkdtemp()
        opts = AdskUsdComponentCreator.Options()
        opts.component_folder = tempDir
        opts.component_name = 'TestComp'
        opts.component_filename = 'TestComp'
        opts.file_extension = 'usda'

        info = AdskUsdComponentCreator.ComponentAPI.CreateFromNothing(opts)
        desc = AdskUsdComponentCreator.ComponentDescription.CreateFromInfo(info)
        vsDesc = AdskUsdComponentCreator.ComponentAPI.AddVariantSet(desc, [], 'model')
        AdskUsdComponentCreator.ComponentAPI.AddVariant(desc, [], vsDesc, 'default')

        rootLayerPath = info.stage.GetRootLayer().realPath

        # Create a proxy shape and point it at the component's root layer.
        transform = cmds.createNode('transform', name='TestComp')
        shape = cmds.createNode('mayaUsdProxyShape', name='TestCompShape', parent=transform)
        cmds.setAttr(shape + '.filePath', rootLayerPath, type='string')

        proxyShapePath = cmds.ls(shape, long=True)[0]
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        return proxyShapePath, desc

    def testDuplicateAsUsdToComponentNoVariantTarget(self):
        '''Duplicate to an Adsk USD Component with no variant targeted — should fail with an error.'''

        cmds.polyCube(name='pCube1')
        cubePath = cmds.ls('pCube1', long=True)[0]

        proxyShapePath, _ = self._createComponent()

        # No variant target is set — duplicate should return False and emit a error.
        with mayaUsd.lib.OpUndoItemList():
            result = mayaUsd.lib.PrimUpdaterManager.duplicate(cubePath, proxyShapePath)
        self.assertFalse(result)

    def testDuplicateAsUsdToComponent(self):
        '''Duplicate a Maya mesh to an Adsk USD Component with a variant targeted.'''
        import AdskUsdComponentCreator

        cmds.polyCube(name='pCube1')
        cubePath = cmds.ls('pCube1', long=True)[0]

        proxyShapePath, desc = self._createComponent()

        # Target the 'default' variant.
        variantSets = desc.GetVariantSets()
        vsDesc = variantSets['model']
        variantDesc = vsDesc.GetVariantDescription('default')
        self.assertTrue(
            AdskUsdComponentCreator.ComponentAPI.SetVariantTarget(desc, [vsDesc, variantDesc]))

        # Duplicate the Maya mesh to the component.
        with mayaUsd.lib.OpUndoItemList():
            result = mayaUsd.lib.PrimUpdaterManager.duplicate(cubePath, proxyShapePath)
        self.assertTrue(result)

        # The cube should now appear in the component's geo scope.
        stage = mayaUsd.ufe.getStage(proxyShapePath)
        self.assertTrue(stage.GetPrimAtPath('/root/geo/pCube1').IsValid())

if __name__ == '__main__':
    fixturesUtils.runTests(globals())
