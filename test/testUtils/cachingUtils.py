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

"""
    Helper functions regarding Cached Playback.
"""
import maya.cmds as cmds

from maya.debug.emModeManager import emModeManager
from maya.plugin.evaluator.CacheEvaluatorManager import CacheEvaluatorManager, CACHE_STANDARD_MODE_EVAL

class NonCachingScope(object):
    '''
    Scope object responsible for setting up non cached mode and restoring default settings after
    '''
    def __enter__(self):
        '''Enter the scope, setting up the evaluator managers and initial states'''
        self.em_mgr = emModeManager()
        self.em_mgr.setMode('emp')
        self.em_mgr.setMode('-cache')

        return self

    def __init__(self, unit_test):
        '''Initialize everything to be empty - only use the "with" syntax with this object'''
        self.em_mgr = None
        self.unit_test = unit_test

    def __exit__(self,exit_type,value,traceback):
        '''Exit the scope, restoring all of the state information'''
        if self.em_mgr:
            self.em_mgr.restore_state()
            self.em_mgr = None

    def verifyScopeSetup(self):
        '''
        Meta-test to check that the scope was defined correctly
        :param unit_test: The test object from which this method was called
        '''
        self.unit_test.assertTrue( cmds.evaluationManager( mode=True, query=True )[0] == 'parallel' )
        if cmds.pluginInfo('cacheEvaluator', loaded=True, query=True):
            self.unit_test.assertFalse( cmds.evaluator( query=True, en=True, name='cache' ) )

    def checkValidFrames(self, expected_valid_frames, layers_mask = 0b01):
        return True

    def waitForCache(self, wait_time=5):
        return

    @staticmethod
    def is_caching_scope():
        '''
        Method to determine whether caching is on or off in this object's scope
        :return: False, since this is the non-caching scope
        '''
        return False

class CachingScope(object):
    '''
    Scope object responsible for setting up caching and restoring original setup after
    '''
    def __enter__(self):
        '''Enter the scope, setting up the evaluator managers and initial states'''
        self.em_mgr = emModeManager()
        self.em_mgr.setMode('emp')
        self.em_mgr.setMode('+cache')
        # Enable idle build to make sure we can rebuild the graph when waiting.
        self.em_mgr.idle_action = emModeManager.idle_action_build

        # Setup caching options
        self.cache_mgr = CacheEvaluatorManager()
        self.cache_mgr.save_state()
        self.cache_mgr.plugin_loaded = True
        self.cache_mgr.enabled = True
        self.cache_mgr.cache_mode = CACHE_STANDARD_MODE_EVAL
        self.cache_mgr.resource_guard = False
        self.cache_mgr.fill_mode = 'syncAsync'

        # Setup autokey options
        self.auto_key_state = cmds.autoKeyframe(q=True, state=True)
        self.auto_key_chars = cmds.autoKeyframe(q=True, characterOption=True)
        cmds.autoKeyframe(e=True, state=False)

        self.waitForCache()

        return self

    def __init__(self, unit_test):
        '''Initialize everything to be empty - only use the "with" syntax with this object'''
        self.em_mgr = None
        self.cache_mgr = None
        self.auto_key_state = None
        self.auto_key_chars = None
        self.unit_test = unit_test

    def __exit__(self,exit_type,value,traceback):
        '''Exit the scope, restoring all of the state information'''
        if self.cache_mgr:
            self.cache_mgr.restore_state()
        if self.em_mgr:
            self.em_mgr.restore_state()
        cmds.autoKeyframe(e=True, state=self.auto_key_state, characterOption=self.auto_key_chars)

    def verifyScopeSetup(self):
        '''
        Meta-test to check that the scope was defined correctly
        :param unit_test: The test object from which this method was called
        '''
        self.unit_test.assertTrue( cmds.evaluationManager( mode=True, query=True )[0] == 'parallel' )
        self.unit_test.assertTrue( cmds.pluginInfo('cacheEvaluator', loaded=True, query=True) )
        self.unit_test.assertTrue( cmds.evaluator( query=True, en=True, name='cache' ) )

    def checkValidFrames(self, expected_valid_frames, layers_mask = 0b01):
        '''
        :param unit_test: The test object from which this method was called
        :param expected_valid_frames: The list of frames the text expected to be cached
        :return: True if the cached frame list matches the expected frame list
        '''
        current_valid_frames = list(self.cache_mgr.get_valid_frames(layers_mask))
        if len(expected_valid_frames) == len(current_valid_frames):
            for current, expected in zip(current_valid_frames,expected_valid_frames):
                if current[0] != expected[0] or current[1] != expected[1]:
                    self.unit_test.fail( "{} != {} (current,expected)".format( current_valid_frames, expected_valid_frames) )
                    return False

            return True
        self.unit_test.fail( "{} != {} (current,expected)".format( current_valid_frames, expected_valid_frames) )
        return False

    def waitForCache(self, wait_time=5):
        '''
        Fill the cache in the background, waiting for a maximum time
        :param unit_test: The test object from which this method was called
        :param wait_time: Time the test is willing to wait for cache completion (in seconds)
        '''
        cmds.currentTime( cmds.currentTime(q=True) )
        cmds.currentTime( cmds.currentTime(q=True) )
        cache_is_ready = cmds.cacheEvaluator( waitForCache=wait_time )
        self.unit_test.assertTrue( cache_is_ready )

    @staticmethod
    def is_caching_scope():
        '''
        Method to determine whether caching is on or off in this object's scope
        :return: True, since this is the caching scope
        '''
        return True
