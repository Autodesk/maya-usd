#
# Copyright 2020 Autodesk
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from __future__ import division

"""
Run a set of performance tests on MayaUsd.

See the class docs for a description of the data collected.

"""

import json
import maya.cmds as cmds
from maya.debug.PlaybackManager import PlaybackManager

# Special value indicating that the operation could not be measured
VALUE_UNAVAILABLE = -1.0

# operations measured
KEY_MAYA_STARTUP = 'maya_startup'
KEY_FIRST_FRAME  = 'first_frame'
KEY_CONSOLIDATE  = 'consolidate'
KEY_TUMBLE       = 'tumble'
KEY_PLAYBACK     = 'playback'
KEY_NEW_SCENE    = 'new_scene'
KEY_SELECT_ALL   = 'select_all'
# values measured
KEY_MEMORY       = 'memory'
KEY_START_FRAME  = 'start_frame'
KEY_END_FRAME    = 'end_frame'
KEY_FRAME_COUNT  = 'frame_count'
KEY_TIME         = 'time'
KEY_ALL_SELECTED = 'all_selected'
KEY_NOT_SELECTED = 'not_selected'

class analyticMayaUsdPerformance():
    """
    The normal output is the set of mayaUsd performance statistics in JSON format.

        {
          "maya_startup"    : {
              "memory"      : [0,0]     # baseline [physical,virtual] memory usage after Maya starts, but before anything is loaded
            }
        , "first_frame" : {
               "memory"    : [0,0]      # memory usage after the scene is loaded & has drawn in the viewport once
             , "time"      : 0          # the time taken to create the proxy shape, load the USD stage and draw it in the viewport
            }
        , "not_selected" : {            # tests run with nothing selected
            "consolidate" : {
                   "memory"    : [0,0]  # memory usage after the scene is consolidated
                 , "time"      : 0      # the time taken to consolidate, not including the 5 slow unconsolidated frames drawn
                }
            , "tumble" : {
                  "memory"    : [0,0]   # memory usage after the scene has been tumbled
                , "time"      : 0       # the total time taken to draw frame_count frames
                , "frame_count" : 0     # the number of frames drawn during the tumble test
                }
            , "playback" : {
                  "memory"    : [0,0]   # memory usage after the scene has been played back
                , "time"      : 0       # the total time taken to playback frame from start_frame to end_frame
                , "start_frame" : 0     # start frame for the playback test.  Time is on start_frame and start_frame has been drawn before the test begins.
                , "end_frame"   : 0     # end frame being used for the playback test.
                }
            }
        , "select_all" : {
              "memory"     : [0,0]      # memory usage after the scene has been selected
            , "time"       : 0          # the total time taken to select the scene
            }
        , "all_selected" : {            # tests run with the full scene selected
            "consolidate" : {
                   "memory"    : [0,0]  # memory usage after the scene is consolidated
                 , "time"      : 0      # the time taken to consolidate, not including the 5 slow unconsolidated frames drawn
                }
            , "tumble" : {
                  "memory"    : [0,0]   # memory usage after the scene has been tumbled
                , "time"      : 0       # the total time taken to draw frame_count frames
                , "frame_count" : 0     # the number of frames drawn during the tumble test
                }
            , "playback" : {
                  "memory"    : [0,0]   # memory usage after the scene has been played back
                , "time"      : 0       # the total time taken to playback frame from start_frame to end_frame
                , "start_frame" : 0     # start frame for the playback test.  Time is on start_frame and start_frame has been drawn before the test begins.
                , "end_frame"   : 0     # end frame being used for the playback test.
                }
            }
        , "new_scene" : {
              "memory"     : [0,0]      # memory usage after the scene has been cleared
            , "time"       : 0          # the total time taken to clear the scene
            }
        }
    """

    def __init__(self):
        """
        Initialize the persistent class members
        """


    #----------------------------------------------------------------------
    @staticmethod
    def get_memory():
        '''
        :return: A 2 member list with current physical and virtual memory in use by Maya
        '''
        return [ cmds.memory(asFloat=True, megaByte=True, physicalMemory=True)
               , cmds.memory(asFloat=True, megaByte=True, adjustedVirtualMemory=True) ]

    #----------------------------------------------------------------------
    def run(self, usdFileName, cameraTranslate, cameraRotate, minTime, maxTime):
        """
        Run the analytic on the current scene.
        Performs scene load, consolidation, tumble, playback selection and unload while
        measuring timing and memory usage.
        
        :result: JSON data as described in the class doc
        """
        json_data = { KEY_MAYA_STARTUP : { KEY_MEMORY      : [0,0]
                                       }
                    , KEY_FIRST_FRAME  : { KEY_MEMORY      : [0,0]
                                         , KEY_TIME        : 0
                                       }
                    , KEY_NOT_SELECTED : { KEY_CONSOLIDATE  : { KEY_MEMORY      : [0,0]
                                                              , KEY_TIME        : 0
                                                            }
                                         , KEY_TUMBLE       : { KEY_MEMORY      : [0,0]
                                                              , KEY_TIME        : 0
                                                              , KEY_FRAME_COUNT : 0
                                                            }
                                         , KEY_PLAYBACK     : { KEY_MEMORY      : [0,0]
                                                              , KEY_TIME        : 0
                                                              , KEY_START_FRAME : 0
                                                              , KEY_END_FRAME   : 0
                                                            }
                                       }
                    , KEY_SELECT_ALL   : { KEY_MEMORY      : [0,0]
                                         , KEY_TIME        : 0
                                       }
                    , KEY_ALL_SELECTED : { KEY_CONSOLIDATE  : { KEY_MEMORY      : [0,0]
                                                              , KEY_TIME        : 0
                                                            }
                                         , KEY_TUMBLE       : { KEY_MEMORY      : [0,0]
                                                              , KEY_TIME        : 0
                                                              , KEY_FRAME_COUNT : 0
                                                            }
                                         , KEY_PLAYBACK     : { KEY_MEMORY      : [0,0]
                                                              , KEY_TIME        : 0
                                                              , KEY_START_FRAME : 0
                                                              , KEY_END_FRAME   : 0
                                                            }
                                       }
                    , KEY_NEW_SCENE    : { KEY_MEMORY      : [0,0]
                                         , KEY_TIME        : 0
                                       }
                    }

        cmds.loadPlugin('mayaUsdPlugin')
        cmds.move(cameraTranslate[0], cameraTranslate[1], cameraTranslate[2], 'persp', absolute=True)
        cmds.rotate(cameraRotate[0], cameraRotate[1], cameraRotate[2], 'persp', absolute=True)
        cmds.setAttr('perspShape.nearClipPlane', 100)
        cmds.setAttr('perspShape.farClipPlane', 1000000)
        cmds.refresh(force=True)

        # maya_startup
        json_data[KEY_MAYA_STARTUP][KEY_MEMORY] = self.get_memory()

        # first_frame
        start_time = cmds.timerX()
        self.createProxyShapeAndLoadUSD(usdFileName)
        cmds.refresh(force=True) #need one refresh to get the first frame time
        json_data[KEY_FIRST_FRAME][KEY_TIME] = cmds.timerX( startTime=start_time )
        json_data[KEY_FIRST_FRAME][KEY_MEMORY] = self.get_memory()

        with PlaybackManager() as play_mgr:
            play_mgr.maxTime = maxTime
            play_mgr.minTime = minTime

            # consolidation, tumble, playback
            self.testViewport(json_data, play_mgr, KEY_NOT_SELECTED)

            # select_all
            start_time = cmds.timerX()
            cmds.select(all=True)
            cmds.refresh(force=True);
            json_data[KEY_SELECT_ALL][KEY_TIME] = cmds.timerX( startTime=start_time )
            json_data[KEY_SELECT_ALL][KEY_MEMORY] = self.get_memory()

            # consolidation, tumble, playback while selected
            self.testViewport(json_data, play_mgr, KEY_ALL_SELECTED)

            #new_scene
            start_time = cmds.timerX()
            cmds.file(newFile=True, force=True)
            json_data[KEY_NEW_SCENE][KEY_TIME] = cmds.timerX( startTime=start_time )
            json_data[KEY_NEW_SCENE][KEY_MEMORY] = self.get_memory()
        
        return json_data

    def testViewport(self, json_data, play_mgr, selected):
        # consolidate
        for i in range(5): #one refresh is triggered outside of testViewport after a change.  Wait 5 frames here.  The next frame is the consolidation frame.
            cmds.refresh(force=True)
        start_time = cmds.timerX()
        cmds.refresh(force=True)
        json_data[selected][KEY_CONSOLIDATE][KEY_TIME] = cmds.timerX( startTime=start_time )
        json_data[selected][KEY_CONSOLIDATE][KEY_MEMORY] = self.get_memory()

        # tumble
        num_frames = int(play_mgr.maxTime) - int(play_mgr.minTime)
        json_data[selected][KEY_TUMBLE][KEY_FRAME_COUNT] = num_frames
        start_time = cmds.timerX()
        for i in range(num_frames-1): # subtract one becuase range is going to give num_frames + 1 value because it starts from 0.
            cmds.refresh(force=True);
        json_data[selected][KEY_TUMBLE][KEY_TIME] = cmds.timerX( startTime=start_time )
        json_data[selected][KEY_TUMBLE][KEY_MEMORY] = self.get_memory()

        # playback
        (elapsed, min_time, max_time) = play_mgr.play_all()
        assert max_time >= min_time
        json_data[selected][KEY_PLAYBACK][KEY_START_FRAME] = min_time
        json_data[selected][KEY_PLAYBACK][KEY_END_FRAME] = max_time
        json_data[selected][KEY_PLAYBACK][KEY_TIME] = elapsed
        json_data[selected][KEY_PLAYBACK][KEY_MEMORY] = self.get_memory()

    def createProxyShapeAndLoadUSD(self, usdFileName):
        #Original script uses mel function 'capitalizeString' to turn the file name (without path)
        #into something usable for the shape name.  I don't care about that here.
        usdNode = cmds.createNode('mayaUsdProxyShape', skipSelect=True, name='UsdStageShape')
        cmds.setAttr(usdNode + '.filePath', usdFileName, type='string')
        cmds.connectAttr('time1.outTime', usdNode+'.time')




