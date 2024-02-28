#!/usr/bin/env mayapy
#
# Copyright 2024 Pixar
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

# these are test modules
import mayaUtils

from maya import cmds
from maya import standalone

import ufe

class testUsdReferenceAssemblyUfeNodes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testFoo(self):
        filePath = "tiny.ma"
        cmds.file(filePath, open=True, force=True)
        a = "|Tiny"
        cmds.assembly(a, edit=True, active='Collapsed')

        proxyPath = '|Tiny|NS_Tiny:CollapsedProxy'
        ms = mayaUtils.createUfePathSegment(proxyPath)
        #us = usdUtils.createUfePathSegment('/A')
        item = ufe.Hierarchy.createItem(ufe.Path([ms]))
        h = ufe.Hierarchy.hierarchy(item)
        children = h.children()
        # Previously, we would populate ufe nodes for *everything* in the usd
        # file.  This was causing performance problems when the usd file was
        # very large but our proxy's primPath was targeting a small subset of
        # the scene.
        #
        # This check makes sure that we're only generating ufe nodes for the
        # prims that are relevant to our prim path.
        self.assertEqual(len(children), 1)
        childUfePath = children[0].path()
        mayaSegment, usdSegment = childUfePath.segments
        self.assertEqual(str(usdSegment), '/A')


if __name__ == '__main__':
    unittest.main(verbosity=2)
