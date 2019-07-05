#!/usr/bin/env python

#-
# ===========================================================================
# Copyright 2018 Autodesk, Inc.  All rights reserved.
#
# Use of this software is subject to the terms of the Autodesk license
# agreement provided at the time of installation or download, or which
# otherwise accompanies this software in either electronic or hard copy form.
# ===========================================================================
#+

#-
# ===========================================================================
#                       WARNING: PROTOTYPE CODE
#
# The code in this file is intended as an engineering prototype, to demonstrate
# UFE integration in Maya.  Its performance and stability may make it
# unsuitable for production use.
#
# Autodesk believes production quality would be better achieved with a C++
# version of this code.
#
# ===========================================================================
#+

'''Universal Front End USD run-time integration.

This module provides integration of the Maya run-time into the Universal
Front End.
'''

import ufe
import usdXformOpCmds

from pxr import Sdf
from pxr import UsdGeom
from pxr import UsdMaya

import maya.cmds as cmds

import re
import sys
from collections import deque

# Maya's UFE run-time name and ID.
kMayaRunTimeName = 'Maya-DG'
mayaRtid = 0

# Register this run-time with UFE under the following name.
kUSDRunTimeName = 'USD'

# Our run-time ID, allocated by UFE at registration time.  Initialize it
# with illegal 0 value.
USDRtid = 0

kIllegalUSDPath = 'Illegal USD run-time path %s.'

kNotGatewayNodePath = 'Tail of path %s is not a gateway node.'

# The normal Maya hierarchy handler, which we decorate for ProxyShape support.
# Keep a reference to it to restore on finalization.
mayaHierarchyHandler = None

# Pixar or AL proxy shape abstraction, to support use of USD proxy shape with
# either plugin.
proxyShapeHandler = None

# FIXME Python binding lifescope.
#
# In code where we return a Python object to C++ code, if we don't keep a
# Python object reference in Python, the Python object is destroyed, and only
# the C++ base class survives.  This "slices" the additional Python derived
# class functionality out of the object, and only the base class functionality
# remains.  See
#
# https://github.com/pybind/pybind11/issues/1145
#
# for a pybind11 pull request that describes the problem and fixes it.  If this
# pull request is merged into pybind11, we should update to that version and
# confirm the fix.
#
# In our case, this situation happens when returning a newly-created Python
# UsdSceneItem directly to C++ code.  Any such code should use the
# keepAlive mechanism.  Note that it is too early to be done in the
# UsdSceneItem __init__, as the keepAlive cleanup code checks that the item
# is in the global selection, which is obviously not the case when still in
# __init__.  PPT, 23-Nov-2017.
#
# Selection items to keep alive.
_keepAlive = {}

def keepAlive(item):
    '''Keep argument item alive by adding it to the keep alive cache.

    Before adding item to the cache, it is cleared of items no longer in the
    selection.
    '''
    global _keepAlive

    # First, clean out cache of items no longer needed.
    # As of 8-Jan-2018, this causes lifescope issues, so "leak" items in the
    # keepAlive map.  Since this Python module is temporary, pending a
    # permanent C++ implementation, not investigating.  PPT.
#     paths = _keepAlive.keys()
#     globalSelection = ufe.GlobalSelection.get()
#     for path in paths:
#         if path not in globalSelection:
#             del _keepAlive[path]

    # Next, add in new item.
    _keepAlive[item.path()] = item

# Map of AL_usdmaya_ProxyShape UFE path to corresponding stage.
pathToStage = {}

def getStage(path):
    '''Get USD stage corresponding to argument Maya Dag path.

    A stage is bound to a single Dag proxy shape.
    '''
    return pathToStage.get(path)

def ufePathToPrim(path):
    '''Return the USD prim corresponding to the argument UFE path.'''
    # Assume that there are only two segments in the path, the first a Maya
    # Dag path segment to the proxy shape, which identifies the stage, and
    # the second the USD segment.
    segments = path.segments
    assert len(segments) == 2, kIllegalUSDPath % str(path)
    dagSegment = segments[0]
    stage = getStage(ufe.Path([dagSegment]))
    return stage.GetPrimAtPath(str(segments[1]))


def isRootChild(path):
    segments = path.segments
    assert len(segments) == 2, kIllegalUSDPath % str(path)
    return len(segments[1]) == 1

def defPrimSpecLayer(prim):
    '''Return the highest-priority layer where the prim has a def primSpec.'''

    # Iterate over the layer stack, starting at the highest-priority layer.
    # The source layer is the one in which there exists a def primSpec, not
    # an over.
    layerStack = prim.GetStage().GetLayerStack()
    defLayer = None
    for layer in layerStack:
        primSpec = layer.GetPrimAtPath(prim.GetPath())
        if primSpec is not None and primSpec.specifier == Sdf.SpecifierDef:
            defLayer = layer
            break
    return defLayer

def createSiblingSceneItem(ufeSrcPath, siblingName):
    ufeSiblingPath = ufeSrcPath.sibling(ufe.PathComponent(siblingName))
    return UsdSceneItem.create(ufeSiblingPath, ufePathToPrim(ufeSiblingPath))

# Compiled regular expression to find a numerical suffix to a path component.
# It searches for any number of characters followed by a single non-numeric,
# then one or more digits at end of string.
_reProg = re.compile('(.*)([^0-9])([0-9]+)$')

def uniqueName(existingNames, srcName):
    # Split the source name into a base name and a numerical suffix (set to
    # 1 if absent).  Increment the numerical suffix until name is unique.
    match = _reProg.search(srcName)
    (base, suffix) = (srcName, 1) if match is None else \
                     (match.group(1)+match.group(2), int(match.group(3))+1)
    dstName = base + str(suffix)
    while dstName in existingNames:
        suffix += 1
        dstName = base + str(suffix)
    return dstName

_inPathChange = False

class InPathChange(object):
    def __enter__(self):
        global _inPathChange
        _inPathChange = True

    def __exit__(self, *args):
        global _inPathChange
        _inPathChange = False

# Provide a single interface for the Pixar proxy shape or the Animal Logic
# proxy shape node.
class ProxyShapeHandlerBase(object):
    def gatewayNodeType(self):
        '''Type of the Maya shape node at the root of a USD hierarchy.'''
        return self.kMayaUsdGatewayNodeType

    def getAllNames(self):
        return cmds.ls(type=self.kMayaUsdGatewayNodeType, long=True)

class PxrProxyShapeHandler(ProxyShapeHandlerBase):
    kMayaUsdGatewayNodeType = 'pxrUsdProxyShape'

    def nameToStage(self, name):
        return UsdMaya.GetPrim(name).GetStage()

    def getAllStages(self):
        # According to Pixar, the following should work:
        # return UsdMaya.StageCache.Get().GetAllStages()
        # but after a file open of a scene with one or more Pixar proxy shapes,
        # returns an empty list.  To be investigated, PPT, 28-Feb-2019.

        # When using the AL plugin, the following line crashes Maya, so it's
        # a Pixar-only solution, not a general one.  PPT, 28-Feb-2019.
        return [UsdMaya.GetPrim(psn).GetStage() for psn in self.getAllNames()]

class ALProxyShapeHandler(ProxyShapeHandlerBase):
    kMayaUsdGatewayNodeType = 'AL_usdmaya_ProxyShape'

    def nameToStage(self, name):
        from AL import usdmaya as AL_USDMaya
        return AL_USDMaya.ProxyShape.getByName(name).getUsdStage()

    def getAllStages(self):
        from AL import usdmaya as AL_USDMaya
        stageCache = AL_USDMaya.StageCache.Get()
        return stageCache.GetAllStages()

def inPathChange():
    return _inPathChange

def uniqueChildName(parent, childPath):
    '''Return a unique child name.

    parent must be a UsdSceneItem.'''
    childrenName = set(child.GetName() for child in parent.prim().GetChildren())
    childName = str(childPath.back())
    if childName in childrenName:
        childName = uniqueName(childrenName, childName)
    return childName

class UsdSceneItem(ufe.SceneItem):
    def __init__(self, path, prim):
        super(UsdSceneItem, self).__init__(path)
        self._prim = prim
        # FIXME Python binding lifescope.
        keepAlive(self)

    @staticmethod
    def create(path, prim):
        '''Create a UsdSceneItem from a UFE path and a USD prim.'''
        # Are we already keeping an item like this alive?  If so, use it.
        item = _keepAlive.get(path)
        if item is None:
            item = UsdSceneItem(path, prim)
        return item

    def prim(self):
        return self._prim

    def nodeType(self):
        return self._prim.GetTypeName()

class UsdHierarchy(ufe.Hierarchy):
    '''USD run-time hierarchy interface.

    This class implements the hierarchy interface for normal USD prims, using
    standard USD calls to obtain a prim's parent and children.
    '''
    def __init__(self):
        super(UsdHierarchy, self).__init__()

    def setItem(self, item):
        self._path = item.path()
        self._prim = item.prim()
        self._item = item

    def sceneItem(self):
        return self._item

    def path(self):
        return self._path

    def hasChildren(self):
        return self._prim.GetChildren() != []

    def parent(self):
        return UsdSceneItem.create(self._path.pop(), self._prim.GetParent())

    def children(self):
        # Return USD children only, i.e. children within this run-time.
        usdChildren = self._prim.GetChildren()

        # The following is the desired code, lifescope issues notwithstanding.
        # children = [UsdSceneItem(self._path + child.GetName(), child)
        #             for child in usdChildren]
        children = [UsdSceneItem.create(self._path + child.GetName(), child)
                    for child in usdChildren]
        return children

    def appendChild(self, child):
        # First, check if we need to rename the child.
        childName = uniqueChildName(self.sceneItem(), child.path())

        # Set up all paths to perform the reparent.
        prim = child.prim()
        stage = prim.GetStage()
        ufeSrcPath = child.path()
        usdSrcPath = prim.GetPath()
        ufeDstPath = self.path() + childName
        usdDstPath = self._prim.GetPath().AppendChild(childName)
        layer = defPrimSpecLayer(prim)
        if layer is None:
            raise RuntimeError("No prim found at %s" % usdSrcPath)

        # In USD, reparent is implemented like rename, using copy to
        # destination, then remove from source.  See
        # UsdUndoRenameCommand._rename comments for details.
        with InPathChange():
            status = Sdf.CopySpec(layer, usdSrcPath, layer, usdDstPath)
            if not status:
                raise RuntimeError("Appending child %s to parent %s failed." %
                                   (str(ufeSrcPath), str(self.path())))

            stage.RemovePrim(usdSrcPath)
            ufeDstItem = UsdSceneItem.create(
                ufeDstPath, ufePathToPrim(ufeDstPath))
            notification = ufe.ObjectReparent(ufeDstItem, ufeSrcPath)
            ufe.Scene.notifyObjectPathChange(notification)

        # FIXME  No idea how to get the child prim index yet.  PPT, 16-Aug-2018.
        return ufe.AppendedChild(ufeDstItem, ufeSrcPath, 0)

    def createGroup(self, name):
        '''Create a transform.'''
        # According to Pixar, the following is more efficient when creating
        # multiple transforms, because of the use of ChangeBlock():
        # with Sdf.ChangeBlock():
        #     primSpec = Sdf.CreatePrimInLayer(layer, usdPath)
        #     primSpec.specifier = Sdf.SpecifierDef
        #     primSpec.typeName = 'Xform'

        # Rename the new group for uniqueness, if needed.
        path = self.path() + name
        childName = uniqueChildName(self.sceneItem(), path)

        # Next, get the stage corresponding to the path.
        segments = path.segments
        assert len(segments) == 2, kIllegalUSDPath % str(path)
        dagSegment = segments[0]
        stage = getStage(ufe.Path([dagSegment]))

        # Build the corresponding USD path and create the USD group prim.
        usdPath = self.sceneItem().prim().GetPath().AppendChild(childName)
        prim = UsdGeom.Xform.Define(stage, usdPath).GetPrim()

        # Create a UFE scene item from the prim.
        ufeChildPath = self.path() + childName
        return UsdSceneItem.create(ufeChildPath, prim)

    # FIXME Python binding lifescope.  See UsdSceneItemOps.undoCommands.
    _MAX_UNDO_COMMANDS = 10000
    undoCommands = deque(maxlen=_MAX_UNDO_COMMANDS)

    def createGroupCmd(self, name):
        createGroupCmd = UsdUndoCreateGroupCommand(self.sceneItem(), name)
        createGroupCmd.execute()
        UsdHierarchy.undoCommands.append(createGroupCmd)
        return ufe.Group(createGroupCmd.group(), createGroupCmd)

class UsdRootChildHierarchy(UsdHierarchy):
    '''USD run-time hierarchy interface for children of the USD root prim.

    This class modifies its base class implementation to return the Maya USD
    gateway node as parent of USD prims that are children of the USD root prim.
    '''
    def __init__(self):
        super(UsdRootChildHierarchy, self).__init__()

    def parent(self):
        # If we're a child of the root, our parent node in the path is a Maya
        # node.  Ask the Maya hierarchy interface to create a selection item
        # for that path.
        parentPath = self._path.pop()
        assert parentPath.runTimeId() == mayaRtid, kNotGatewayNodePath % str(path)
        
        mayaHierarchyHandler = ufe.RunTimeMgr.instance().hierarchyHandler(mayaRtid)
        return mayaHierarchyHandler.createItem(parentPath)

class UsdHierarchyHandler(ufe.HierarchyHandler):
    '''USD run-time hierarchy handler.

    This hierarchy handler is the standard USD run-time hierarchy handler.  Its
    only special behavior is to return a UsdRootChildHierarchy interface object
    if it is asked for a hierarchy interface for a child of the USD root prim.
    These prims are special because we define their parent to be the Maya USD
    gateway node, which the UsdRootChildHierarchy interface implements.
    '''
    def __init__(self):
        super(UsdHierarchyHandler, self).__init__()
        self._usdRootChildHierarchy = UsdRootChildHierarchy()
        self._usdHierarchy = UsdHierarchy()

    def hierarchy(self, item):
        if isRootChild(item.path()):
            self._usdRootChildHierarchy.setItem(item)
            return self._usdRootChildHierarchy
        else:
            self._usdHierarchy.setItem(item)
            return self._usdHierarchy

    def createItem(self, path):
        return UsdSceneItem.create(path, ufePathToPrim(path))

class ProxyShapeHierarchy(ufe.Hierarchy):
    '''USD gateway node hierarchy interface.

    This class defines a hierarchy interface for a single kind of Maya node,
    the USD gateway node.  This node is special in that its parent is a Maya
    node, but its children are children of the USD root prim.
    '''
    def __init__(self, mayaHierarchyHandler):
        super(ProxyShapeHierarchy, self).__init__()
        self._mayaHierarchyHandler = mayaHierarchyHandler

    def setItem(self, item):
        self._item = item
        self._mayaHierarchy = self._mayaHierarchyHandler.hierarchy(item)
        self._usdRootPrim = None

    def _getUsdRootPrim(self):
        if self._usdRootPrim is None:
            try:
                self._usdRootPrim = getStage(self._item.path()).GetPrimAtPath('/');
            except:
                # FIXME During AL_usdmaya_ProxyShapeImport, nodes (both Maya
                # and USD) are being added (e.g. the proxy shape itself), but
                # there is no stage yet, and there is no way to detect that a
                # proxy shape import command is under way.  PPT, 28-Sep-2018.
                pass
        return self._usdRootPrim

    def hasChildren(self):
        rootPrim = self._getUsdRootPrim()
        if not rootPrim:
            return False
        return len(rootPrim.GetChildren()) > 0

    def parent(self):
        return self._mayaHierarchy.parent()

    def children(self):
        # Return children of the USD root.
        rootPrim = self._getUsdRootPrim()
        if not rootPrim:
            return []
        usdChildren = rootPrim.GetChildren()
        parentPath = self._item.path()

        # We must create selection items for our children.  These will have as
        # path the path of the proxy shape, with a single path segment of a
        # single component appended to it.
        # The following is the desired code, lifescope issues notwithstanding.
        # children = [UsdSceneItem(parentPath + ufe.PathSegment(
        #     ufe.PathComponent(child.GetName()), USDRtid, '/'), child) for child
        #             in usdChildren]
        children = [UsdSceneItem.create(parentPath + ufe.PathSegment(
            ufe.PathComponent(child.GetName()), USDRtid, '/'), child) for child
                    in usdChildren]
        return children

class ProxyShapeHierarchyHandler(ufe.HierarchyHandler):
    '''Maya run-time hierarchy handler with support for USD gateway node.

    This hierarchy handler is NOT a USD run-time hierarchy handler: it is a
    Maya run-time hierarchy handler.  It decorates the standard Maya run-time
    hierarchy handler and replaces it, providing special behavior only if the
    requested hierarchy interface is for the Maya to USD gateway node.  In that
    case, it returns a special ProxyShapeHierarchy interface object, which
    knows how to handle USD children of the Maya ProxyShapeHierarchy node.

    For all other Maya nodes, this hierarchy handler simply delegates the work
    to the standard Maya hierarchy handler it decorates, which returns a
    standard Maya hierarchy interface object.
    '''
    def __init__(self, mayaHierarchyHandler):
        super(ProxyShapeHierarchyHandler, self).__init__()
        self._mayaHierarchyHandler = mayaHierarchyHandler
        self._proxyShapeHierarchy = ProxyShapeHierarchy(mayaHierarchyHandler)

    def hierarchy(self, item):
        if item.nodeType() == proxyShapeHandler.gatewayNodeType():
            self._proxyShapeHierarchy.setItem(item)
            return self._proxyShapeHierarchy
        else:
            return self._mayaHierarchyHandler.hierarchy(item)

    def createItem(self, path):
        return self._mayaHierarchyHandler.createItem(path)

class UsdTransform3d(ufe.Transform3d):
    def __init__(self):
        super(UsdTransform3d, self).__init__()
    
    def setItem(self, item):
        self._prim = item.prim()
        self._path = item.path()
        self._item = item
        
    def sceneItem(self):
        return self._item
  
    def path(self):
        return self._path

    def translate(self, x, y, z):
        usdXformOpCmds.translateOp(self._prim, self._path, x, y, z)
        
    def translation(self):
        x, y, z = (0, 0, 0)
        if self._prim.HasAttribute('xformOp:translate'):
            # Initially, attribute can be created, but have no value.
            v = self._prim.GetAttribute('xformOp:translate').Get()
            if v is not None:
                x, y, z = v
        return ufe.Vector3d(x, y, z)
        
    # FIXME Python binding lifescope.  Memento objects are returned directly to
    # C++, which does not keep them alive.  Use LIFO deque with maximum size to
    # keep them alive without consuming an unbounded amount of memory.  This
    # hack will fail for more than _MAX_MEMENTOS objects.  PPT, 12-Dec-2017.
    _MAX_MEMENTOS = 10000
    mementos = deque(maxlen=_MAX_MEMENTOS)

        
    def translateCmd(self):
        # FIXME Python binding lifescope.  Must keep command object alive.
        translateCmd = usdXformOpCmds.UsdTranslateUndoableCommand(self._prim, self._path, self._item)
        UsdTransform3d.mementos.append(translateCmd)
        return translateCmd
    
    def rotate(self, x, y, z):
        usdXformOpCmds.rotateOp(self._prim, self._path, x, y, z)
        
    def rotateCmd(self):
        # FIXME Python binding lifescope.  Must keep command object alive.
        rotateCmd = usdXformOpCmds.UsdRotateUndoableCommand(self._prim, self._path, self._item)
        UsdTransform3d.mementos.append(rotateCmd)
        return rotateCmd
        
    def scale(self, x, y, z):
        usdXformOpCmds.scaleOp(self._prim, self._path, x, y, z)
        
    def scaleCmd(self):
        # FIXME Python binding lifescope.  Must keep command object alive.
        scaleCmd = usdXformOpCmds.UsdScaleUndoableCommand(self._prim, self._path, self._item)
        UsdTransform3d.mementos.append(scaleCmd)
        return scaleCmd

    def rotatePivotTranslate(self, x, y, z):
        usdXformOpCmds.rotatePivotTranslateOp(self._prim, self._path, x, y, z)
        
    def rotatePivotTranslateCmd(self):
        # FIXME Python binding lifescope.  Must keep command object alive.
        translateCmd = usdXformOpCmds.UsdRotatePivotTranslateUndoableCommand(self._prim, self._path, self._item)
        UsdTransform3d.mementos.append(translateCmd)
        return translateCmd

    def rotatePivot(self):
        x, y, z = (0, 0, 0)
        if self._prim.HasAttribute('xformOp:translate:pivot'):
            # Initially, attribute can be created, but have no value.
            v = self._prim.GetAttribute('xformOp:translate:pivot').Get()
            if v is not None:
                x, y, z = v
        return ufe.Vector3d(x, y, z)
        
    def scalePivot(self):
        return self.rotatePivot()
        
    def scalePivotTranslate(self, x, y, z):
        return self.rotatePivotTranslate(x, y, z)


    def segmentInclusiveMatrix(self):
        return usdXformOpCmds.primToUfeXform(self._prim)
        
    def segmentExclusiveMatrix(self):
        return usdXformOpCmds.primToUfeExclusiveXform(self._prim)

class UsdTransform3dHandler(ufe.Transform3dHandler):
    def __init__(self):
        super(UsdTransform3dHandler, self).__init__()
        self._usdTransform3d = UsdTransform3d()

    def transform3d(self, item):
        self._usdTransform3d.setItem(item)
        return self._usdTransform3d

class UsdUndoDeleteCommand(ufe.UndoableCommand):
    def __init__(self, prim):
        super(UsdUndoDeleteCommand, self).__init__()
        self._prim = prim

    def _perform(self, state):
        self._prim.SetActive(state)

    def undo(self):
        self._perform(True)

    def redo(self):
        self._perform(False)

class UsdUndoDuplicateCommand(ufe.UndoableCommand):

    def __init__(self, srcPrim, ufeSrcPath):
        super(UsdUndoDuplicateCommand, self).__init__()
        self._srcPrim = srcPrim
        self._stage = srcPrim.GetStage()
        self._ufeSrcPath = ufeSrcPath
        (self._usdDstPath, self._layer) = self.primInfo(srcPrim)

    def usdDstPath(self):
        return self._usdDstPath

    @staticmethod
    def primInfo(srcPrim):
        '''Return the USD destination path and layer.'''
        parent = srcPrim.GetParent()
        childrenName = set(child.GetName() for child in parent.GetChildren())
        # Find a unique name for the destination.  If the source name already
        # has a numerical suffix, increment it, otherwise append "1" to it.
        dstName = uniqueName(childrenName, srcPrim.GetName())
        usdDstPath = parent.GetPath().AppendChild(dstName)
        # Iterate over the layer stack, starting at the highest-priority layer.
        # The source layer is the one in which there exists a def primSpec, not
        # an over.  An alternative would have beeen to call Sdf.CopySpec for
        # each layer in which there is an over or a def, until we reach the
        # layer with a def primSpec.  This would preserve the visual appearance
        # of the duplicate.  PPT, 12-Jun-2018.
        srcLayer = defPrimSpecLayer(srcPrim)
        if srcLayer is None:
            raise RuntimeError("No prim found at %s" % srcPrim.GetPath())
        return (usdDstPath, srcLayer)

    @staticmethod
    def duplicate(layer, usdSrcPath, usdDstPath):
        '''Duplicate the prim hierarchy at usdSrcPath.

        Returns True for success.
        '''
        # We use the source layer as the destination.  An alternate workflow
        # would be the edit target layer be the destination:
        # layer = self._stage.GetEditTarget().GetLayer()
        return Sdf.CopySpec(layer, usdSrcPath, layer, usdDstPath)

    def undo(self):
        # USD sends a ResyncedPaths notification after the prim is removed, but
        # at that point the prim is no longer valid, and thus a UFE post delete
        # notification is no longer possible.  To respect UFE object delete
        # notification semantics, which require the object to be alive when
        # the notification is sent, we send a pre delete notification here.
        notification = ufe.ObjectPreDelete(createSiblingSceneItem(
            self._ufeSrcPath, self._usdDstPath.elementString))
        ufe.Scene.notifyObjectDelete(notification)

        self._stage.RemovePrim(self._usdDstPath)

    def redo(self):
        # MAYA-92264: Pixar bug prevents redo from working.  Try again with USD
        # version 0.8.5 or later.  PPT, 28-May-2018.
        try:
            self.duplicate(self._layer, self._srcPrim.GetPath(), self._usdDstPath)
        except Exception as e:
            print e
            raise

class UsdUndoRenameCommand(ufe.UndoableCommand):

    def __init__(self, srcItem, newName):
        super(UsdUndoRenameCommand, self).__init__()
        prim = srcItem.prim()
        self._stage = prim.GetStage()
        self._ufeSrcPath = srcItem.path()
        self._usdSrcPath = prim.GetPath()
        # Every call to rename() (through execute(), undo() or redo()) removes
        # a prim, which becomes expired.  Since USD UFE scene items contain a
        # prim, we must recreate them after every call to rename.
        self._ufeDstItem = None
        self._usdDstPath = prim.GetParent().GetPath().AppendChild(str(newName))
        self._layer = defPrimSpecLayer(prim)
        if self._layer is None:
            raise RuntimeError("No prim found at %s" % prim.GetPath())

    def renamedItem(self):
        return self._ufeDstItem

    def rename(self, layer, ufeSrcPath, usdSrcPath, usdDstPath):
        with InPathChange():
            self._rename(layer, ufeSrcPath, usdSrcPath, usdDstPath)

    def _rename(self, layer, ufeSrcPath, usdSrcPath, usdDstPath):
        '''Rename the prim hierarchy at usdSrcPath to usdDstPath.'''
        # We use the source layer as the destination.  An alternate workflow
        # would be the edit target layer be the destination:
        # layer = self._stage.GetEditTarget().GetLayer()
        status = Sdf.CopySpec(layer, usdSrcPath, layer, usdDstPath)
        if status:
            self._stage.RemovePrim(usdSrcPath)
            # The renamed scene item is a "sibling" of its original name.
            self._ufeDstItem = createSiblingSceneItem(
                ufeSrcPath, usdDstPath.elementString)
            # USD sends two ResyncedPaths() notifications, one for the CopySpec
            # call, the other for the RemovePrim call (new name added, old name
            # removed).  Unfortunately, the rename semantics are lost: there is
            # no notion that the two notifications belong to the same atomic
            # change.  Provide a single Rename notification ourselves here.
            notification = ufe.ObjectRename(self._ufeDstItem, ufeSrcPath)
            ufe.Scene.notifyObjectPathChange(notification)

        return status

    def undo(self):
        # MAYA-92264: Pixar bug prevents undo from working.  Try again with USD
        # version 0.8.5 or later.  PPT, 7-Jul-2018.
        try:
            self.rename(self._layer, self._ufeDstItem.path(), self._usdDstPath,
                        self._usdSrcPath)
        except Exception as e:
            print e
            raise

    def redo(self):
        self.rename(self._layer, self._ufeSrcPath, self._usdSrcPath,
                    self._usdDstPath)

class UsdUndoCreateGroupCommand(ufe.UndoableCommand):

    def __init__(self, parentItem, name):
        super(UsdUndoCreateGroupCommand, self).__init__()
        self._parentItem = parentItem
        self._name = name
        self._group = None

    def group(self):
        return self._group

    def undo(self):
        if self._group is None:
            return

        # See UsdUndoDuplicateCommand.undo() comments.
        notification = ufe.ObjectPreDelete(self._group)
        ufe.Scene.notifyObjectDelete(notification)

        prim    = self._group.prim()
        stage   = prim.GetStage()
        usdPath = prim.GetPath()
        stage.RemovePrim(usdPath)

        self._group = None

    def redo(self):
        hierarchy = ufe.Hierarchy.hierarchy(self._parentItem)
        # See MAYA-92264: redo doesn't work.  PPT, 19-Nov-2018.
        self._group = hierarchy.createGroup(self._name)

class UsdSceneItemOps(ufe.SceneItemOps):
    def __init__(self):
        super(UsdSceneItemOps, self).__init__()

    def setItem(self, item):
        self._prim = item.prim()
        self._path = item.path()
        self._item = item

    def sceneItem(self):
        return self._item

    def path(self):
        return self._path

    # FIXME Python binding lifescope.  Command objects are returned directly to
    # C++, which does not keep them alive.  Use LIFO deque with maximum size to
    # keep them alive without consuming an unbounded amount of memory.  This
    # hack will fail for more than _MAX_UNDO_COMMANDS objects.
    # PPT, 1-May-2018.
    _MAX_UNDO_COMMANDS = 10000
    undoCommands = deque(maxlen=_MAX_UNDO_COMMANDS)

    def deleteItem(self):
        return self._prim.SetActive(False)

    def deleteItemCmd(self):
        # FIXME Python binding lifescope.  Must keep command object alive.
        deleteCmd = UsdUndoDeleteCommand(self._prim)
        deleteCmd.execute()
        UsdSceneItemOps.undoCommands.append(deleteCmd)
        return deleteCmd

    def duplicateItem(self):
        (usdDstPath, layer) = UsdUndoDuplicateCommand.primInfo(self._prim)
        status = UsdUndoDuplicateCommand.duplicate(
            layer, self._prim.GetPath(), usdDstPath)
        # The duplicate is a sibling of the source.
        return createSiblingSceneItem(
            self.path(), usdDstPath.elementString) if status else None

    def duplicateItemCmd(self):
        # FIXME Python binding lifescope.  Must keep command object alive.
        duplicateCmd = UsdUndoDuplicateCommand(self._prim, self._path)
        duplicateCmd.execute()
        UsdSceneItemOps.undoCommands.append(duplicateCmd)
        return ufe.Duplicate(createSiblingSceneItem(
            self.path(), duplicateCmd.usdDstPath().elementString), duplicateCmd)

    def renameItem(self, newName):
        UsdUndoRenameCommand(self._item, newName).execute()

    def renameItemCmd(self, newName):
        # FIXME Python binding lifescope.  Must keep command object alive.
        renameCmd = UsdUndoRenameCommand(self._item, newName)
        renameCmd.execute()
        UsdSceneItemOps.undoCommands.append(renameCmd)
        return ufe.Rename(renameCmd.renamedItem(), renameCmd)

class UsdSceneItemOpsHandler(ufe.SceneItemOpsHandler):
    def __init__(self):
        super(UsdSceneItemOpsHandler, self).__init__()
        self._usdSceneItemOps = UsdSceneItemOps()

    def sceneItemOps(self, item):
        self._usdSceneItemOps.setItem(item)
        return self._usdSceneItemOps

def initialize():
    # Replace the Maya hierarchy handler with ours.
    global mayaHierarchyHandler
    global mayaRtid
    global USDRtid
    mayaRtid = ufe.RunTimeMgr.instance().getId(kMayaRunTimeName)
    mayaHierarchyHandler = ufe.RunTimeMgr.instance().hierarchyHandler(mayaRtid)
    ufe.RunTimeMgr.instance().setHierarchyHandler(
        mayaRtid, ProxyShapeHierarchyHandler(mayaHierarchyHandler))

    USDRtid = ufe.RunTimeMgr.instance().register(
        kUSDRunTimeName, UsdHierarchyHandler(),
        UsdTransform3dHandler(), UsdSceneItemOpsHandler())

    # Set up to support either the Pixar proxy shape or the AL proxy shape.  In
    # order:
    # 1) If AL plugin is loaded, use AL proxy shape.
    # 2) If Pixar plugin is loaded, use Pixar proxy shape.
    # 3) Default: use AL proxy shape.
    global proxyShapeHandler
    if cmds.pluginInfo("AL_USDMayaPlugin", query=True, loaded=True):
        proxyShapeHandler = ALProxyShapeHandler()
    elif cmds.pluginInfo("pxrUsd", query=True, loaded=True):
        proxyShapeHandler = PxrProxyShapeHandler()
    else:
        proxyShapeHandler = ALProxyShapeHandler()

def finalize():
    # Restore the normal Maya hierarchy handler, and unregister.
    global mayaHierarchyHandler
    ufe.RunTimeMgr.instance().setHierarchyHandler(mayaRtid, mayaHierarchyHandler)
    ufe.RunTimeMgr.instance().unregister(USDRtid)
    mayaHierarchyHandler = None

    global proxyShapeHandler
    proxyShapeHandler = None
