#!/usr/bin/env mayapy
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

import mayaUsd.lib

from pxr import Usd

from maya import cmds
from maya import standalone

import fixturesUtils, os
import mayaUsd_createStageWithNewLayer
import mayaUsdDuplicateAsUsdDataOptions

import unittest


class ChaserExample1(mayaUsd.lib.ExportChaser):
    '''
    Chaser that will be requested by job context example 1.
    '''
    # Chaser name, used in the 'chaser' entry of the user-args dictionary of the export,
    # so that the chaser is invoked during an export.
    name = 'ChaserExample1'

    chaserArgName = 'arg1'
    chaserArgValue = 7

    # Record which functions got called, used to assert during the test.
    exportDefaultCalled = False
    exportFrameCalled = False
    postExportCalled = False

    # These will be filled when the chaser is created so that we can assert in the test.
    seenChasers = None
    seenChaserArgs = None

    def __init__(self, factoryContext, *args, **kwargs):
        super(ChaserExample1, self).__init__(factoryContext, *args, **kwargs)
        jobArgs = factoryContext.GetJobArgs()
        ChaserExample1.seenChasers = jobArgs.chaserNames
        if ChaserExample1.name in jobArgs.allChaserArgs:
            ChaserExample1.seenChaserArgs = jobArgs.allChaserArgs[ChaserExample1.name]
        
    def ExportDefault(self):
        ChaserExample1.exportDefaultCalled = True
        return self.ExportFrame(Usd.TimeCode.Default())

    def ExportFrame(self, frame):
        ChaserExample1.exportFrameCalled = True
        return True

    def PostExport(self):
        ChaserExample1.postExportCalled = True
        return True

    @staticmethod
    def register():
        '''
        Register the chaser with its name.
        '''
        mayaUsd.lib.ExportChaser.Register(ChaserExample1, ChaserExample1.name)


class JobContextExample1:
    '''
    Export job context example 1.

    Will request that the example chaser 1 be used by setting it in the 'chaser'
    entry of the settings it returns in its enabler function.
    '''
    # Exporter plugin description: job context, UI name, description.
    jobContextName = 'JobContextExample1'
    friendlyName = 'This is example-1'
    description = 'JobContextExample1 will export this or that'

    # Exporter plugin settings.
    # In a real plugin, should be saved on-disk, maybe using Maya option vars.
    strideValue = 5.0
    strideToken = 'frameStride'
    customValue = 1.
    customToken = 'JobContextExample1-custom-setting'

    @staticmethod
    def exportEnablerFn():
        '''
        The exporter plugin settings callback, returns the settings it want forced with the forced value.
        '''
        settings = {
            JobContextExample1.strideToken: JobContextExample1.strideValue,
            JobContextExample1.customToken: JobContextExample1.customValue,
            'chaser': [ChaserExample1.name],
        }
        return settings


    @staticmethod
    def register():
        '''
        Register the exporter plugin with an exporter settings callback and a UI callback.
        '''
        mayaUsd.lib.JobContextRegistry.RegisterExportJobContext(
            JobContextExample1.jobContextName, JobContextExample1.friendlyName, JobContextExample1.description,
            JobContextExample1.exportEnablerFn)


class ChaserExample2(mayaUsd.lib.ExportChaser):
    '''
    Chaser that will be requested by job context example 2.
    '''
    # Chaser name, used in the 'chaser' entry of the user-args dictionary of the export,
    # so that the chaser is invoked during an export.
    name = 'ChaserExample2'

    chaserArgName = 'arg1'
    chaserArgValue = 7

    # Record which functions got called, used to assert during the test.
    exportDefaultCalled = False
    exportFrameCalled = False
    postExportCalled = False

    # These will be filled when the chaser is created so that we can assert in the test.
    seenChasers = None
    seenChaserArgs = None

    def __init__(self, factoryContext, *args, **kwargs):
        super(ChaserExample2, self).__init__(factoryContext, *args, **kwargs)
        self.stage = factoryContext.GetStage()
        jobArgs = factoryContext.GetJobArgs()
        ChaserExample2.seenChasers = jobArgs.chaserNames
        if ChaserExample2.name in jobArgs.allChaserArgs:
            ChaserExample2.seenChaserArgs = jobArgs.allChaserArgs[ChaserExample2.name]
        
    def ExportDefault(self):
        ChaserExample2.exportDefaultCalled = True
        return self.ExportFrame(Usd.TimeCode.Default())

    def ExportFrame(self, frame):
        ChaserExample2.exportFrameCalled = True
        return True

    def PostExport(self):
        ChaserExample2.postExportCalled = True

        # creating an extra prim to be tested on Duplicate As
        scope = Usd.Prim = self.stage.DefinePrim("/TestScope", "Scope")
        self.RegisterExtraPrimsPaths([scope.GetPath()])

        return True

    @staticmethod
    def register():
        '''
        Register the chaser with its name.
        '''
        mayaUsd.lib.ExportChaser.Register(ChaserExample2, ChaserExample2.name)


class JobContextExample2:
    '''
    Export job context example 2.

    Will request that the example chaser 2 be used by setting it in the 'chaser'
    entry of the settings it returns in its enabler function.
    '''
    # Exporter plugin description: job context, UI name, description.
    jobContextName = 'JobContextExample2'
    friendlyName = 'This is example-2'
    description = 'JobContextExample2 will export this or that'

    @staticmethod
    def exportEnablerFn():
        '''
        The exporter plugin settings callback, returns the settings it want forced with the forced value.
        '''
        settings = {
            'chaser': [ChaserExample2.name],
        }
        return settings


    @staticmethod
    def register():
        '''
        Register the exporter plugin with an exporter settings callback and a UI callback.
        '''
        mayaUsd.lib.JobContextRegistry.RegisterExportJobContext(
            JobContextExample2.jobContextName, JobContextExample2.friendlyName, JobContextExample2.description,
            JobContextExample2.exportEnablerFn)


class TestExportChaserWithJobContext(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixturesUtils.setUpClass(__file__)
        cls.temp_dir = os.path.abspath('.')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testSimpleExportChaser(self):
        ChaserExample1.register()
        ChaserExample2.register()
        JobContextExample1.register()
        JobContextExample2.register()

        cmds.polySphere(r = 3.5, name='apple')

        usdFilePath = os.path.join(self.temp_dir,'testExportChaser.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            jobContext=[JobContextExample1.jobContextName, JobContextExample2.jobContextName],
            chaserArgs=[
                (ChaserExample1.name, ChaserExample1.chaserArgName, ChaserExample1.chaserArgValue),
                (ChaserExample2.name, ChaserExample2.chaserArgName, ChaserExample2.chaserArgValue),
            ],
            shadingMode='none')
        
        self.assertIn(ChaserExample1.name, ChaserExample1.seenChasers)
        self.assertIn(ChaserExample1.chaserArgName, ChaserExample1.seenChaserArgs)

        self.assertTrue(ChaserExample1.exportDefaultCalled)
        self.assertTrue(ChaserExample1.exportFrameCalled)
        self.assertTrue(ChaserExample1.postExportCalled)

        self.assertIn(ChaserExample2.name, ChaserExample2.seenChasers)
        self.assertIn(ChaserExample2.chaserArgName, ChaserExample2.seenChaserArgs)

        self.assertTrue(ChaserExample2.exportDefaultCalled)
        self.assertTrue(ChaserExample2.exportFrameCalled)
        self.assertTrue(ChaserExample2.postExportCalled)

    def testChaserWithDuplicateAsUsd(self):
        ChaserExample1.register()
        JobContextExample1.register()

        sphere = cmds.polySphere(r = 1, name='apple')

        # Create a stage to receive the USD duplicate.
        psPathStr = mayaUsd_createStageWithNewLayer.createStageWithNewLayer()
        defaultDuplicateAsUsdDataOptions = mayaUsdDuplicateAsUsdDataOptions.getDuplicateAsUsdDataOptionsText()
        modifiedDuplicateAsUsdDataOptions = defaultDuplicateAsUsdDataOptions.replace("jobContext=[]", "jobContext=[JobContextExample1]")
        cmds.mayaUsdDuplicate(cmds.ls(sphere, long=True)[0], psPathStr, exportOptions=modifiedDuplicateAsUsdDataOptions)

        # check if the extra prim has also been duplicated
        stage = mayaUsd.lib.GetPrim(psPathStr).GetStage()
        applePrim = stage.GetPrimAtPath("/apple")
        scopePrim = stage.GetPrimAtPath("/TestScope")
        
        self.assertTrue(applePrim.IsValid())
        self.assertTrue(scopePrim.IsValid())


if __name__ == '__main__':
    unittest.main(verbosity=2)
