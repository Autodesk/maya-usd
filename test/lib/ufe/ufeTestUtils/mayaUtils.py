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
import re

import ufe

mayaRuntimeID = 1
mayaSeparator = "|"

prRe = re.compile('Preview Release ([0-9]+)')

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
        print(sys.exc_info()[1])
        print("Unable to load %s" % pluginName)
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

    # TODO: HS, June 10, 2020 investigate why x needs to be encoded
    if sys.version_info[0] == 2:
        return [x.encode('UTF8') for x in cmds.ls(sl=True)]
    else:
        return [x for x in cmds.ls(sl=True)]

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

def openCylinderScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "cylinder", "usdCylinder.ma" )
    cmds.file(filePath, force=True, open=True)

def openTwoSpheresScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "twoSpheres", "twoSpheres.ma" )
    cmds.file(filePath, force=True, open=True)

def openSphereAnimatedRadiusScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "sphereAnimatedRadius", "sphereAnimatedRadiusProxyShape.ma" )
    cmds.file(filePath, force=True, open=True)

def openTreeScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "tree", "tree.ma" )
    cmds.file(filePath, force=True, open=True)

def openTreeRefScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "tree", "treeRef.ma" )
    cmds.file(filePath, force=True, open=True)

def openAppleBiteScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "appleBite", "appleBite.ma" )
    cmds.file(filePath, force=True, open=True)

def openGroupBallsScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "groupBalls", "ballset.ma" )
    cmds.file(filePath, force=True, open=True)

def openPrimitivesScene():
    filePath = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "test-samples", "reorderCmd", "primitives.ma" )
    cmds.file(filePath, force=True, open=True)

def previewReleaseVersion():
    '''Return the Maya Preview Release version.

    If the version of Maya is 2019, returns 98.

    If the version of Maya is 2020, returns 110.

    If the version of Maya is current and is not a Preview Release, returns
    sys.maxsize (a very large number).  If the environment variable
    MAYA_PREVIEW_RELEASE_VERSION_OVERRIDE is defined, return its value instead.
    '''

    if 'MAYA_PREVIEW_RELEASE_VERSION_OVERRIDE' in os.environ:
        return int(os.environ['MAYA_PREVIEW_RELEASE_VERSION_OVERRIDE'])

    majorVersion = int(cmds.about(majorVersion=True))
    if majorVersion == 2019:
        return 98
    elif majorVersion == 2020:
        return 110

    match = prRe.match(cmds.about(v=True))

    return int(match.group(1)) if match else sys.maxsize
