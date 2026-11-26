#!/usr/bin/env mayapy
#
# Copyright 2026 Sony Interactive Entertainment
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

"""
Checks that a schema API adaptor plugin registered with USD after a lookup is
still discovered.

The schema API adaptor registry caches each Maya type's adaptors, including
ones inherited from an ancestor type. When USD registers new plugins, that
cache must be cleared, otherwise the type keeps its inherited adaptor and the
new plugin is never loaded.
"""

import json
import os
import sys
import unittest

import mayaUsd.lib as mayaUsdLib

from pxr import Plug

from maya import cmds
from maya import standalone

import fixturesUtils

_PLUGIN_MODULE = 'pointLightShadowAdaptorPlugin'

# The plugin's module: registers a pointLight ShadowAPI adaptor when imported.
_PLUGIN_SOURCE = '''
import mayaUsd.lib as mayaUsdLib

class PointLightShadowAdaptor(mayaUsdLib.SchemaApiAdaptor):
    def CanAdapt(self):
        return True

mayaUsdLib.SchemaApiAdaptor.Register(PointLightShadowAdaptor, "pointLight", "ShadowAPI")
'''


def _writePlugin(directory):
    """Writes a Python USD plugin that provides the pointLight adaptor. With no
    mayaPlugin entry, discovery loads it through USD, which imports the module."""
    os.makedirs(directory, exist_ok=True)
    with open(os.path.join(directory, _PLUGIN_MODULE + '.py'), 'w') as f:
        f.write(_PLUGIN_SOURCE)

    plugInfo = {'Plugins': [{
        'Type': 'python',
        'Name': _PLUGIN_MODULE,
        'Info': {'UsdMaya': {'SchemaApiAdaptor': {'providesTranslator': ['pointLight']}}},
    }]}
    with open(os.path.join(directory, 'plugInfo.json'), 'w') as f:
        json.dump(plugInfo, f)


class LightShadowAdaptor(mayaUsdLib.SchemaApiAdaptor):
    """Inherited by every light type so a pointLight only gets
    ShadowAPI from the plugin's adaptor."""
    calls = 0

    def CanAdapt(self):
        LightShadowAdaptor.calls += 1
        return False


class testSchemaApiAdaptorPluginDiscovery(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testPluginRegisteredAfterLookup(self):
        cmds.file(new=True, force=True)
        mayaUsdLib.SchemaApiAdaptor.Register(LightShadowAdaptor, 'light', 'ShadowAPI')
        lightShape = cmds.pointLight()

        # pointLight has no adaptor of its own, therefore the lookup resolves, and
        # caches, the one inherited from light.
        self.assertEqual(mayaUsdLib.Adaptor(lightShape).GetAppliedSchemas(), [])
        self.assertTrue(LightShadowAdaptor.calls)

        pluginDir = os.path.abspath('PointLightShadowAdaptorPlugin')
        _writePlugin(pluginDir)
        sys.path.insert(0, pluginDir)
        Plug.Registry().RegisterPlugins(pluginDir)

        # Registering the plugin cleared the cache, so this lookup discovers
        # and loads it, and the pointLight adaptor wins over the inherited one.
        self.assertEqual(mayaUsdLib.Adaptor(lightShape).GetAppliedSchemas(), ['ShadowAPI'])
        self.assertIn(_PLUGIN_MODULE, sys.modules)


if __name__ == '__main__':
    unittest.main(verbosity=2)
