#!/usr/bin/env mayapy
#
# Copyright 2022 Autodesk
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

from maya import cmds
from maya import standalone
import  mayaUsd.ufe

import fixturesUtils
import unittest

class testUfeUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSanitizeName(self):
        """
        Test the sanitizeName function.
        """
        testNames = [
            ('Good', 'Good'),
            ('No spaces', 'No_spaces'),
            ('39NoStartDigits', 'NoStartDigits'),
            ('39No start digits or spaces', 'No_start_digits_or_spaces'),
            ('TrailingDigitsOkay24', 'TrailingDigitsOkay24'),
            ('Inside78DigitsOkay', 'Inside78DigitsOkay'),
            ('NoSpecial~!@#$Chars1', 'NoSpecial_Chars1'),
            ('NoSpecial%^&*()Chars2', 'NoSpecial_Chars2'),
            ('NoSpecial-=+,.?`Chars3', 'NoSpecial_Chars3'),
            ('NoSpecial:{}|<>Chars4', 'NoSpecial_Chars4'),
            ('NoSpecial\'\"Chars5', 'NoSpecial_Chars5'),
            ('NoSpecial[]/Chars6', 'NoSpecial_Chars6'),
            ('NoBackslash\\7', 'NoBackslash_7'),
            ]
        for oneName in testNames:
            sn = mayaUsd.ufe.sanitizeName(oneName[0])
            self.assertEqual(sn, oneName[1])


if __name__ == '__main__':
    unittest.main(verbosity=2)
