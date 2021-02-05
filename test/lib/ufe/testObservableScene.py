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

import fixturesUtils

from maya import standalone

import ufe

import os
import sys
import unittest


class TestObserver(ufe.Observer):
    def __init__(self):
        self.add = 0
        self.delete = 0
        self.pathChange = 0
        self.subtreeInvalidate = 0
        self.composite = 0
        super(TestObserver, self).__init__()

    def __call__(self, notification):
        if isinstance(notification, ufe.ObjectAdd):
            self.add += 1
        if isinstance(notification, ufe.ObjectDelete):
            self.delete += 1
        if isinstance(notification, ufe.ObjectPathChange):
            self.pathChange += 1
        if isinstance(notification, ufe.SubtreeInvalidate):
            self.subtreeInvalidate += 1
        if isinstance(notification, ufe.SceneCompositeNotification):
            self.composite += 1

    def notifications(self):
        return [self.add, self.delete, self.pathChange, self.subtreeInvalidate, self.composite]

class UFEObservableSceneTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        fixturesUtils.readOnlySetUpClass(__file__, loadPlugin=False)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def checkNotifications(self, testObserver, listNotifications):
        self.assertTrue(testObserver.add == listNotifications[0])
        self.assertTrue(testObserver.delete == listNotifications[1])
        self.assertTrue(testObserver.pathChange == listNotifications[2])
        self.assertTrue(testObserver.subtreeInvalidate == listNotifications[3])
        self.assertTrue(testObserver.composite == listNotifications[4])

    def testObservableScene(self):
        # Setup
        ca = ufe.PathComponent("a")
        cb = ufe.PathComponent("b")
        cc = ufe.PathComponent("c")

        sa = ufe.PathSegment([ca], 3, '/')
        sab = ufe.PathSegment([ca, cb], 1, '|')
        sc = ufe.PathSegment([cc], 2, '/')

        a = ufe.Path(sa)
        b = ufe.Path(sab)
        c = ufe.Path([sab, sc])

        itemA = ufe.SceneItem(a)
        itemB = ufe.SceneItem(b)
        itemC = ufe.SceneItem(c)
        # End Setup

        # No observers yet.
        self.assertEqual(ufe.Scene.nbObservers(), 0)

        snObs = TestObserver()

        # Add observer to the scene.
        ufe.Scene.addObserver(snObs)

        # Order of expected notifications. No notifications yet.
        self.checkNotifications(snObs, [0,0,0,0,0,0])

        self.assertEqual(ufe.Scene.nbObservers(), 1)
        self.assertTrue(ufe.Scene.hasObserver(snObs))

        ufe.Scene.notify(ufe.ObjectAdd(itemA))

        # we should now have an ObjectAdd notification
        self.checkNotifications(snObs, [1,0,0,0,0,0])

        # HS 2020: can't pass ufe.scene to NotificationGuard??
        
        # Composite notifications with guard.
        # with ufe.NotificationGuard(ufe.Scene):
        #     ufe.Scene.notify(ufe.ObjectAdd(itemB))
        #     ufe.Scene.notify(ufe.ObjectAdd(itemC))


if __name__ == '__main__':
    unittest.main(verbosity=2)
