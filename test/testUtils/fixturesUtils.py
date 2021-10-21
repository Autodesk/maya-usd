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

from maya import cmds

import os
import shutil
import sys
import unittest

_exportTranslatorName = "USD Export"

def _setUpClass(modulePathName, initializeStandalone, loadPlugin):
    '''
    Common code for setUpClass() and readOnlySetUpClass()
    '''
    if initializeStandalone:
        from maya import standalone
        standalone.initialize('usd')

    if loadPlugin:
        cmds.loadPlugin('mayaUsdPlugin', quiet=True)

        # Monkey patch cmds so that usdExport and usdImport becomes aliases. We
        # *only* do this if we're loading the plugin, since otherwise the
        # export/import commands will not exist.
        cmds.usdExport = cmds.mayaUSDExport
        cmds.usdImport = cmds.mayaUSDImport

    realPath = os.path.realpath(modulePathName)
    return os.path.split(realPath)

def exportTranslatorName():
    return _exportTranslatorName

def setUpClass(modulePathName, suffix='', initializeStandalone=True,
        loadPlugin=True):
    '''
    Test class setup.

    This function:
    - (Optionally) Initializes Maya standalone use.
    - Creates (or empties) a test output directory based on the argument.
    - Changes the current working directory to the test output directory.
    - (Optionally) Loads the plugin
    - Returns the original directory from the argument.
    '''
    (testDir, testFile) = _setUpClass(modulePathName, initializeStandalone,
        loadPlugin)
    outputName = os.path.splitext(testFile)[0]+suffix+'Output'

    outputPath = os.path.join(os.path.abspath('.'), outputName)
    if os.path.exists(outputPath):
        # Remove previous test run output.
        shutil.rmtree(outputPath)

    os.mkdir(outputPath)
    os.chdir(outputPath)

    return testDir

def readOnlySetUpClass(modulePathName, initializeStandalone=True,
        loadPlugin=True):
    '''
    Test class import setup for tests that do not write to the file system.

    This function:
    - (Optionally) Initializes Maya standalone use.
    - (Optionally) Loads the plugin
    - Returns the original directory from the argument.
    '''
    (testDir, testFile) = _setUpClass(modulePathName, initializeStandalone,
        loadPlugin)

    return testDir

def loadTestsFromDict(namespace_dict):
    '''
    Returns a unittest.TestSuite object with tests loaded from the given dict

    Similar to unittest.TestLoader.loadTestsFromModule, but works off a dict
    rather than a module object. Useful when running from inside a "script"
    context where there is no module, but are globals().

    Examples
    --------
    >>> testSuite = loadTestsFromDict(globals())
    '''
    # just piggy-back on loadTestsFromModule, since all it does is dir() and
    # getattr checks...
    class DummyModule(object):
        __name__

    dummyModule = DummyModule()

    for name, val in namespace_dict.items():
        if not name.startswith('__'):
            setattr(dummyModule, name, val)
    return unittest.TestLoader().loadTestsFromModule(dummyModule)

def runTests(globals_dict, stream=sys.__stderr__,
             verbosity=1):
    '''
    Run the unittests within the given namespace

    Intended usage:
        import fixturesUtils
        if __name__ == '__main__':
            fixturesUtils.runTests(globals())
    '''
    import maya.cmds as cmds
    suite = loadTestsFromDict(globals_dict)
    runner = unittest.TextTestRunner(stream=stream, verbosity=verbosity)
    results = runner.run(suite)
    if results.wasSuccessful():
        exitCode = 0
    else:
        exitCode = 1

    # cmds.quit will not flush the streams - make sure we do so!
    # ...flush all of the standard ones just to be sure, as well as the stream
    # given (which probably means it will be flushed twice, but that's fine)
    sys.stdout.flush()
    sys.stderr.flush()
    sys.__stdout__.flush()
    sys.__stderr__.flush()
    stream.flush()

    # maya running interactively will absorb much of the output. comment out the
    # following to prevent maya from exiting and open the script editor to look
    # at failures.
    cmds.quit(abort=True, exitCode=exitCode)