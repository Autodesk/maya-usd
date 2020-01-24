#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

from maya import cmds
from maya import standalone


class testPxrUsdTranslators(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testLoads(self):
        # The pxrUsdTranslators plugin does absolutely nothing, registers no
        # nodes, or commands.  Pending explanation from Pixar as to what its
        # purpose is, remove the load test of this plugin.  PPT, 2-Oct-2019.
        # self.assertEqual(cmds.loadPlugin('pxrUsdTranslators'),
        #         ["pxrUsdTranslators"])
        pass

if __name__ == '__main__':
    unittest.main(verbosity=2)
