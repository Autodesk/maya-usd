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
import shutil
import tempfile

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

class TemporaryDirectory:
    '''
    Context manager that creates a temporary directory and deletes it on exit,
    so it's usable with "with" statement.
    '''
    def __init__(self, suffix=None, prefix=None, ignore_errors=True, keep_files=False):
        # Note: the default for suffix and prefix changed between Maya 2.7 and 3.X
        #       from empty strings to None. To be compatible with both, we won't
        #       pass values when we want the default, so we have these awkward if elif.
        if suffix and prefix:
            self.name = tempfile.mkdtemp(suffix=suffix, prefix=prefix)
        elif suffix:
            self.name = tempfile.mkdtemp(suffix=suffix)
        elif prefix:
            self.name = tempfile.mkdtemp(prefix=prefix)
        else:
            self.name = tempfile.mkdtemp()
        self.ignore_errors = ignore_errors
        self.keep_files = keep_files

    def __enter__(self):
        return self.name

    def __exit__(self, exc_type, exc_value, traceback):
        if self.keep_files:
            return
        try:
            shutil.rmtree(self.name)
        except:
            if not self.ignore_errors:
                raise
