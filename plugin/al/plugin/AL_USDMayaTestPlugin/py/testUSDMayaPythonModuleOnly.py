#!/usr/bin/env python

#
# Copyright 2020 Autodesk
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

'''Test that basic functionality of the python module works without maya.
'''

# Practically speaking, this test is currently just to check that linking of
# the python binary is set up correctly, since there's not any real reason
# to import this module without maya. However, we do at least want it to
# import without error.

import unittest
import tempfile

class TestPythonModule(unittest.TestCase):

    def testImport(self):
        """
        Test that we can import the AL.usdmaya module, and that it links correctly
        """
        import sys
        self.assertNotIn('maya.standalone', sys.modules)
        self.assertNotIn('AL', sys.modules)
        self.assertNotIn('AL.usdmaya', sys.modules)
        import AL.usdmaya
        self.assertIn('AL', sys.modules)
        self.assertIn('AL.usdmaya', sys.modules)

    def test_ExportFlag(self):
        """
        Test ExportFlag enum.
        """
        import AL.usdmaya
        self.assertTrue(hasattr(AL.usdmaya, 'ExportFlag'))
        self.assertTrue(hasattr(AL.usdmaya.ExportFlag, 'kNotSupported'))
        self.assertEqual(AL.usdmaya.ExportFlag.values[0],
                         AL.usdmaya.ExportFlag.kNotSupported)
        self.assertTrue(hasattr(AL.usdmaya.ExportFlag, 'kFallbackSupport'))
        self.assertEqual(AL.usdmaya.ExportFlag.values[1],
                         AL.usdmaya.ExportFlag.kFallbackSupport)
        self.assertTrue(hasattr(AL.usdmaya.ExportFlag, 'kSupported'))
        self.assertEqual(AL.usdmaya.ExportFlag.values[2],
                         AL.usdmaya.ExportFlag.kSupported)


if __name__ == "__main__":
    tests = unittest.TestLoader().loadTestsFromTestCase(TestPythonModule)
    result = unittest.TextTestRunner(verbosity=2).run(tests)
    exit(not result.wasSuccessful())