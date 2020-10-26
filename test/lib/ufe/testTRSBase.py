#!/usr/bin/env python

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

import unittest

import maya.cmds as cmds

from testUtils import assertVectorAlmostEqual

class TRSTestCaseBase(unittest.TestCase):
    '''Base class for translate (move), rotate, scale command tests.'''

    def snapShotAndTest(self, expected, places=7):
        '''Take a snapshot of the state and append it to the memento list.

        The snapshot is compared to the expected results.
        '''
        snapshot = self.snapshotRunTimeUFE()
        self.memento.append(snapshot)
        # Since snapshotRunTimeUFE checks run-time to UFE equality, we can use
        # the UFE or the run-time value to check against expected.
        assertVectorAlmostEqual(self, snapshot[1], expected, places)
            
    def multiSelectSnapShotAndTest(self, items, expected, places=7):
        '''Take a snapshot of the state and append it to the memento list.

        The snapshot is compared to the expected results.
        '''
        snapshot = self.multiSelectSnapshotRunTimeUFE(items)
        self.memento.append(snapshot)
        for (itemSnapshot, itemExpected) in zip(snapshot, expected):
            # Since multiSelectSnapshotRunTimeUFE checks run-time to UFE
            # equality, we can use the UFE or the run-time value to
            # check against expected.
            assertVectorAlmostEqual(self, itemSnapshot[1], itemExpected, places)
            
    def rewindMemento(self):
        '''Undo through all items in memento.

        Starting at the top of the undo stack, perform undo to bring us to the
        bottom of the undo stack.  During iteration, we ensure that the current
        state matches the state stored in the memento.
        '''
        # Ignore the top of the memento stack, as it corresponds to the current
        # state.
        for m in reversed(self.memento[:-1]):
            cmds.undo()
            self.assertEqual(m, self.snapshotRunTimeUFE())
            
    def fforwardMemento(self):
        '''Redo through all items in memento.

        Starting at the bottom of the undo stack, perform redo to bring us to
        the top of the undo stack.  During iteration, we ensure that the current
        state matches the state stored in the memento.
        '''
        # For a memento list of length n, n-1 redo operations sets us current.
        self.assertEqual(self.memento[0], self.snapshotRunTimeUFE())
        # Skip first
        for m in self.memento[1:]:
            cmds.redo()
            self.assertEqual(m, self.snapshotRunTimeUFE())
        
    def multiSelectRewindMemento(self, items):
        '''Undo through all items in memento.

        Starting at the top of the undo stack, perform undo to bring us to the
        bottom of the undo stack.  During iteration, we ensure that the current
        state matches the state stored in the memento.
        '''
        # Ignore the top of the memento stack, as it corresponds to the current
        # state.
        for m in reversed(self.memento[:-1]):
            cmds.undo()
            self.assertEqual(m, self.multiSelectSnapshotRunTimeUFE(items))
            
    def multiSelectFforwardMemento(self, items):
        '''Redo through all items in memento.

        Starting at the bottom of the undo stack, perform redo to bring us to
        the top of the undo stack.  During iteration, we ensure that the current
        state matches the state stored in the memento.
        '''
        # For a memento list of length n, n-1 redo operations sets us current.
        self.assertEqual(self.memento[0], self.multiSelectSnapshotRunTimeUFE(items))
        # Skip first
        for m in self.memento[1:]:
            cmds.redo()
            self.assertEqual(m, self.multiSelectSnapshotRunTimeUFE(items))
