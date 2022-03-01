#!/usr/bin/env python

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

import mayaUsdOptions
import unittest

from testUtils import assertVectorAlmostEqual, getTestScene

import os

class testMayaUsdOptions(unittest.TestCase):
    '''
    Test mayaUsdOptions helper functions.
    '''

    def assertTextContains(self, text, expectedParts):
        '''
        Verify if the expected text parts are in the text as column-separated items.
        '''
        parts = text.split(';')
        for expected in expectedParts:
            self.assertTrue(expected in parts)

    def testconvertOptionsDictToText(self):
        '''
        Test Conversion of options dictionary to text.
        '''

        simpleDict = {
            'integer': 1,
            'float': 0.1,
            'text': 'hello',
            'truth': True,
            'lies': False,
        }

        simpleText = mayaUsdOptions.convertOptionsDictToText(simpleDict)
        self.assertTextContains(simpleText, ['integer=1', 'float=0.1', 'text=hello', 'truth=1', 'lies=0'])

        compositeDict = {
            'floats': [1.0, 2.0],
            'texts': ['hello', 'goodbye'],
            'empty': [],
        }

        compositeText = mayaUsdOptions.convertOptionsDictToText(compositeDict)
        self.assertTextContains(compositeText, ['floats=1.0 2.0', 'texts=hello,goodbye', 'empty='])

    def convertOptionsTextToDict(self):
        '''
        Test Conversion of options text to dictionary.
        '''

        defaultDict = {
            'integer': 1,
            'float': 1.,
            'text': '',
            'truth': bool,
            'lies': bool,
            'floats': [],
            'texts': [],
            'empty': [],
        }

        simpleText = ';'.join(['integer=1', 'float=0.1', 'text=hello', 'truth=1', 'lies=0'])
        simpleDict = mayaUsdOptions.convertOptionsTextToDict(simpleText, defaultDict)
        self.assertEqual(simpleDict, {
            'integer': 1,
            'float': 0.1,
            'text': 'hello',
            'truth': True,
            'lies': False,
        })

        compositeText = ';'.join(['floats=1.0 2.0   0.0', 'texts=hello, goodbye', 'empty='])


        compositeDict = mayaUsdOptions.convertOptionsTextToDict(compositeText, defaultDict)
        self.assertEqual(compositeDict, {
            'floats': [1.0, 2.0],
            'texts': ['hello', 'goodbye'],
            'empty': [],
        })


if __name__ == '__main__':
    unittest.main(verbosity=2)
