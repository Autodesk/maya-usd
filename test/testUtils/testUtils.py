#
# Copyright 2019 Autodesk
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
    General test utilities. The functions here should not use Maya, Ufe or Usd.
"""

import os


def stripPrefix(input_str, prefix):
    if input_str.startswith(prefix):
        return input_str[len(prefix):]
    return input_str

def assertMatrixAlmostEqual(testCase, ma, mb, places=7):
    for ra, rb in zip(ma, mb):
        for a, b in zip(ra, rb):
            testCase.assertAlmostEqual(a, b, places)

def assertVectorAlmostEqual(testCase, a, b, places=7):
    for va, vb in zip(a, b):
        testCase.assertAlmostEqual(va, vb, places)

def assertVectorNotAlmostEqual(testCase, a, b, places=7):
    for va, vb in zip(a, b):
        testCase.assertNotAlmostEqual(va, vb, places)

def assertVectorEqual(testCase, a, b):
    for va, vb in zip(a, b):
        testCase.assertEqual(va, vb)

def getTestScene(*args):
    return os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "testSamples", *args)

def filterUsdStr(usdSceneStr):
    '''Remove empty lines and lines starting with pound character.'''
    nonBlankLines = filter(None, [l.rstrip() for l in usdSceneStr.splitlines()])
    finalLines = [l for l in nonBlankLines if not l.startswith('#')]
    return '\n'.join(finalLines)
