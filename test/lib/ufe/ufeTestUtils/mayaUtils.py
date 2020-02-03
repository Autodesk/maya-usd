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

"""
    Helper functions regarding Maya that will be used throughout the test.
"""


import maya.cmds as cmds
import sys, os

import ufe

mayaRuntimeID = 1
mayaSeparator = "|"

def loadPlugin(pluginName):
    """ 
        Load all given plugins created or needed by maya-ufe-plugin 
        Args:
            pluginName (str): The plugin name to load
        Returns:
            True if all plugins are loaded. False if a plugin failed to load
    """
    try:
        if not isPluginLoaded(pluginName):
            cmds.loadPlugin( pluginName, quiet = True )
        return True
    except:
        print sys.exc_info()[1]
        print "Unable to load %s" % pluginName
        return False
            
def isPluginLoaded(pluginName):
    """ 
        Verifies that the given plugin is loaded
        Args:
            pluginName (str): The plugin name to verify
        Returns:
            True if the plugin is loaded. False if a plugin failed to load
    """
    return cmds.pluginInfo( pluginName, loaded=True, query=True)
    
def isMayaUsdPluginLoaded():
    """ 
        Load plugins needed by UFE tests.
        Returns:
            True if plugins loaded successfully. False if a plugin failed to load
    """
    # Load the mayaUsdPlugin first.
    if not loadPlugin("mayaUsdPlugin"):
        return False

    # Load the UFE support plugin, for ufeSelectCmd support.  If this plugin
    # isn't included in the distribution of Maya (e.g. Maya 2019 or 2020), use
    # fallback test plugin.
    if not (loadPlugin("ufeSupport") or loadPlugin("ufeTestCmdsPlugin")):
        return False

    # The renderSetup Python plugin registers a file new callback to Maya.  On
    # test application exit (in TbaseApp::cleanUp()), a file new is done and
    # thus the file new callback is invoked.  Unfortunately, this occurs after
    # the Python interpreter has been finalized, which causes a crash.  Since
    # renderSetup is not needed for mayaUsd tests, unload it.
    rs = 'renderSetup'
    if cmds.pluginInfo(rs, q=True, loaded=True):
        unloaded = cmds.unloadPlugin(rs)
        return (unloaded[0] == rs)
    
    return True

def createUfePathSegment(mayaPath):
    """
        Create an UFE path from a given maya path.
        Make sure that it starts with |world. We are currently 
        supporting Maya nodes being at the top of UFE Paths (03/26/2018)
        Args:
            mayaPath (str): The maya path to use
        Returns :
            PathSegment of the given mayaPath
    """
    if not mayaPath.startswith("|world"):
        mayaPath = "|world" + mayaPath
    return ufe.PathSegment(mayaPath, mayaRuntimeID, mayaSeparator)

    
def getMayaSelectionList():
    """ 
        Returns the current Maya selection in a list
        Returns:
            A list(str) containing all selected Maya items
    """
    # Remove the unicode of cmds.ls
    return [x.encode('UTF8') for x in cmds.ls(sl=True)]
    
def openTopLayerScene():
    '''
        The test scene hierarchy is represented as :
                |world
                    |pSphere1
                        |pSphereShape1
                    |transform1
                        |proxyShape1
                            /Room_set
                                /Props
                                    /Ball_1
                                    /Ball_2
                                    ...
                                    /Ball_35
    '''
    # Open top_layer file which contains the USD scene
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "ballset", "StandaloneScene", "top_layer.ma" )
    cmds.file(filePath, force=True, open=True)
    
