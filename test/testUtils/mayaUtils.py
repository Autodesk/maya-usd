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

from math import radians
from mayaUsd import lib as mayaUsdLib
from mayaUsd import ufe as mayaUsdUfe

from maya import cmds
from maya.api import OpenMaya as om

import ufe
import ufeUtils, testUtils

import os
import re
import sys

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
        Create a UFE path from a given maya path and return the first segment.
        Args:
            mayaPath (str): The maya path to use
        Returns :
            PathSegment of the given mayaPath
    """
    if ufeUtils.ufeFeatureSetVersion() >= 2:
        return ufe.PathString.path(mayaPath).segments[0]
    else:
        if not mayaPath.startswith("|world"):
            mayaPath = "|world" + mayaPath
        return ufe.PathSegment(mayaPath, mayaUsdUfe.getMayaRunTimeId(),
            mayaSeparator)

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

def openTestScene(*args):
    filePath = testUtils.getTestScene(*args)
    cmds.file(filePath, force=True, open=True)

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
    return openTestScene("ballset", "StandaloneScene", "top_layer.ma" )

def openCylinderScene():
    return openTestScene("cylinder", "usdCylinder.ma" )

def openTwoSpheresScene():
    return openTestScene("twoSpheres", "twoSpheres.ma" )

def openSphereAnimatedRadiusScene():
    return openTestScene("sphereAnimatedRadius", "sphereAnimatedRadiusProxyShape.ma" )

def openTreeScene():
    return openTestScene("tree", "tree.ma" )

def openTreeRefScene():
    return openTestScene("tree", "treeRef.ma" )

def openAppleBiteScene():
    return openTestScene("appleBite", "appleBite.ma" )

def openGroupBallsScene():
    return openTestScene("groupBalls", "ballset.ma" )

def openPrimitivesScene():
    return openTestScene("reorderCmd", "primitives.ma" )

def openPointInstancesGrid14Scene():
    return openTestScene("pointInstances", "PointInstancer_Grid_14.ma" )

def openPointInstancesGrid7kScene():
    return openTestScene("pointInstances", "PointInstancer_Grid_7k.ma" )

def openPointInstancesGrid70kScene():
    return openTestScene("pointInstances", "PointInstancer_Grid_70k.ma" )

def openVariantSetScene():
    return openTestScene("variantSet", "Variant.ma" )

def openCompositionArcsScene():
    return openTestScene("compositionArcs", "compositionArcs.ma" )

def openPrimPathScene():
    return openTestScene("primPath", "primPath.ma" )

def setMayaTranslation(aMayaItem, t):
    '''Set the translation on the argument Maya scene item.'''

    aMayaPath = aMayaItem.path()
    aMayaPathStr = ufe.PathString.string(aMayaPath)
    aDagPath = om.MSelectionList().add(aMayaPathStr).getDagPath(0)
    aFn= om.MFnTransform(aDagPath)
    aFn.setTranslation(t, om.MSpace.kObject)
    return (aMayaPath, aMayaPathStr, aFn, aFn.transformation().asMatrix())

def setMayaRotation(aMayaItem, r):
    '''Set the rotation (XYZ) on the argument Maya scene item.'''

    aMayaPath = aMayaItem.path()
    aMayaPathStr = ufe.PathString.string(aMayaPath)
    aDagPath = om.MSelectionList().add(aMayaPathStr).getDagPath(0)
    aFn = om.MFnTransform(aDagPath)
    rads = [ radians(v) for v in r ]
    rot = om.MEulerRotation(rads[0], rads[1], rads[2])
    aFn.setRotation(rot, om.MSpace.kTransform)
    return (aMayaPath, aMayaPathStr, aFn, aFn.transformation().asMatrix())

def createProxyAndStage():
    """
    Create in-memory stage
    """
    cmds.createNode('mayaUsdProxyShape', name='stageShape')

    shapeNode = cmds.ls(sl=True,l=True)[0]
    shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()
    
    cmds.select( clear=True )
    cmds.connectAttr('time1.outTime','{}.time'.format(shapeNode))

    return shapeNode,shapeStage

def createProxyFromFile(filePath):
    """
    Load stage from file
    """
    cmds.createNode('mayaUsdProxyShape', name='stageShape')

    shapeNode = cmds.ls(sl=True,l=True)[0]
    cmds.setAttr('{}.filePath'.format(shapeNode), filePath, type='string')
    
    shapeStage = mayaUsdLib.GetPrim(shapeNode).GetStage()
    
    cmds.select( clear=True )
    cmds.connectAttr('time1.outTime','{}.time'.format(shapeNode))

    return shapeNode,shapeStage

def createSingleSphereMayaScene(directory=None):
    '''Create a Maya scene with a single polygonal sphere.
    Returns the file path.
    '''

    cmds.file(new=True, force=True)
    cmds.CreatePolygonSphere()
    tempMayaFile = 'simpleSphere.ma'
    if directory is not None:
        tempMayaFile = os.path.join(directory, tempMayaFile)
    # Prevent Windows single backslash from being interpreted as a control
    # character.
    tempMayaFile = tempMayaFile.replace(os.sep, '/')
    cmds.file(rename=tempMayaFile)
    cmds.file(save=True, force=True, type='mayaAscii')
    return tempMayaFile

def previewReleaseVersion():
    '''Return the Maya Preview Release version.

    If the version of Maya is 2019, returns 98.

    If the version of Maya is 2020, returns 110.

    If the version of Maya is 2022, returns 122.

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
    elif majorVersion == 2022:
        return 122

    match = prRe.match(cmds.about(v=True))

    return int(match.group(1)) if match else sys.maxsize

def mayaMajorVersion():
    return int(cmds.about(majorVersion=True))

def mayaMinorVersion():
    return int(cmds.about(minorVersion=True))

def mayaMajorMinorVersions():
    """
    Return the Maya version as a tuple (Major, Minor).
    Thanks to Python tuple comparison rules, (2022, 0) > (2021,3).
    """
    return (mayaMajorVersion(), mayaMinorVersion())

def activeModelPanel():
    """Return the model panel that will be used for playblasting etc..."""
    for panel in cmds.getPanel(type="modelPanel"):
        if cmds.modelEditor(panel, q=1, av=1):
            return panel
