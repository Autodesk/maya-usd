#!/usr/bin/env python

#
# Copyright 2024 Autodesk
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

import fixturesUtils
import mayaUtils
import testUtils
import ufeUtils

from maya import cmds
from maya import standalone

import ufe

import unittest


def extractTranslationFromMatrix4d(matrix4d):
    translation = matrix4d.matrix[-1]
    return translation[:-1]

def getTransform3dMatrix4d(transform3d):
    return transform3d.inclusiveMatrix()


class Transform3dTranslateTimeSampleTestCase(unittest.TestCase):
    '''
    Verify that translating a USD reference that has its current
    transform defined in a matrix *and* is time-sample does not
    lose the other aspects of the matrix.
    '''

    pluginsLoaded = False
    
    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        ''' Called initially to set up the maya test environment '''
        # Load plugins
        self.assertTrue(mayaUtils.isMayaUsdPluginLoaded())
        
    def testTranslateTimeSampleMatrix(self):
        '''Move Maya object, read through the Transform3d interface.'''

        proxyNode, stage = mayaUtils.createProxyAndStage()
        testFileName = testUtils.getTestScene("timeSamples", 'timeSamples.usda')
        stage.GetRootLayer().subLayerPaths.append(testFileName)

        withTimeSampleUsdPath = '/building/withTimeSamples'
        primWithTimeSamples = stage.GetPrimAtPath(withTimeSampleUsdPath)
        self.assertTrue(primWithTimeSamples)
        itemWithTimeSamples = ufeUtils.createItem(ufe.PathString.path(proxyNode + ',' + withTimeSampleUsdPath))

        # Extract the intial matrix and compare all but the translation (last row)
        transform3d = ufe.Transform3d.transform3d(itemWithTimeSamples)
        initialMatrix = getTransform3dMatrix4d(transform3d)
        initialTranslation = extractTranslationFromMatrix4d(initialMatrix)

        def verifyMatrix(expectedTranslation):
            currentMatrix = getTransform3dMatrix4d(transform3d)
            currentTranslation = extractTranslationFromMatrix4d(currentMatrix)
            for col in range(0, 3):
                self.assertAlmostEqual(expectedTranslation[col], currentTranslation[col],
                                       msg='Translation differ: %s !- %s' % (currentTranslation, expectedTranslation))
            # Note: don't compare row 4, since that is where the translation is.
            for row in range(0, 3):
                for col in range(0, 4):
                    self.assertAlmostEqual(currentMatrix.matrix[row][col], initialMatrix.matrix[row][col],
                                           msg='Current and initial matrix differ: %s != %s' % (currentMatrix.matrix, initialMatrix.matrix))

        verifyMatrix(initialTranslation)
        
        # Select the item so it can be translated with cmds.
        ufe.GlobalSelection.get().append(itemWithTimeSamples)

        # Do some move commands, and compare.
        for xlation in [[4, 5, 6], [-3, -2, -1]]:
            cmds.move(*xlation)
            verifyMatrix(xlation)


if __name__ == '__main__':
    unittest.main(verbosity=2)
