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

'''Universal Front End USD Observer pattern experiment.

This experimental module is to investigate providing UFE Observer support
for USD, that is, to translate USD notifications into a UFE equivalent.'''

import maya.api.OpenMaya as OpenMaya
import maya.cmds as cmds
# For nameToDagPath().
import maya.app.renderSetup.common.utils as utils

from pxr import Tf
from pxr import Usd

import ufe
import usdRunTime

# Subject singleton for observation of all USD stages.
_stagesSubject = None

def dagPathToUfe(dagPath):
    # This function can only create UFE Maya scene items with a single segment,
    # as it is only given a Dag path as input.
    return ufe.Path(dagPathToPathSegment(dagPath));

def dagPathToPathSegment(dagPath):
    # Unfortunately, there does not seem to be a Maya API equivalent to
    # TdagPathIterator.  There must be a better way.  PPT, 3-Oct-2018.
    nbComponents = dagPath.length()
    components = []
    for i in xrange(nbComponents):
        obj = dagPath.node()
        fn = OpenMaya.MFnDependencyNode(obj)
        components.insert(0, ufe.PathComponent(fn.name()))
        dagPath.pop()
    # Prepend world at the start of the segment.
    components.insert(0, ufe.PathComponent('world'))
    return ufe.PathSegment(components, 1, '|');

# Map of stage to corresponding AL_usdmaya_ProxyShape UFE path.  Ideally, we
# would support dynamically computing the path for the AL_usdmaya_ProxyShape
# node, but we assume here it will not be reparented.  We will also assume that
# a USD stage will not be instanced (even though nothing in the data model
# prevents it).
_stageToPath = {}

def stagePath(stage):
    '''Return the AL_usdmaya_ProxyShape node UFE path for the argument stage.'''
    # Use get() to return None rather than raise KeyError.
    return _stageToPath.get(stage)

def isTransform3D(usdChangedPath):
    '''Return whether a USD changed path notification denotes a UFE
    Transform3d change.'''
    # Very rough initial implementation.  For now, compare USD path element
    # string value.
    return usdChangedPath.elementString == '.xformOp:translate'

class _StagesSubject(object):
    '''Subject class to observe Maya scene.

    This class observes Maya file open, to register a USD observer on each
    stage the Maya scene contains.  This USD observer translates USD
    notifications into UFE notifications.
    '''

    def __init__(self):
        super(_StagesSubject, self).__init__()

        # Map of per-stage listeners, indexed by stage.
        self._stageListeners = {}

        # Workaround to MAYA-65920: at startup, MSceneMessage.kAfterNew file
        # callback is incorrectly called by Maya before the
        # MSceneMessage.kBeforeNew file callback, which should be illegal.
        # Detect this and ignore illegal calls to after new file callbacks.
        # PPT, 19-Jan-16.
        self._beforeNewCbCalled = False

        self._cbIds = []
        cbArgs = [(OpenMaya.MSceneMessage.kBeforeNew, self._beforeNewCb,
                   'before new'),
                  (OpenMaya.MSceneMessage.kBeforeOpen, self._beforeOpenCb, 
                   'before open'),
                  (OpenMaya.MSceneMessage.kAfterOpen, self._afterOpenCb, 
                   'after open'),
                  (OpenMaya.MSceneMessage.kAfterNew, self._afterNewCb, 
                   'after new')]

        for (msg, cb, data) in cbArgs:
            self._cbIds.append(
                OpenMaya.MSceneMessage.addCallback(msg, cb, data))

    def finalize(self):
        OpenMaya.MMessage.removeCallbacks(self._cbIds)
        self._cbIds = []

    def _beforeNewCb(self, data):
        self._beforeNewCbCalled = True

    def _beforeOpenCb(self, data):
        self._beforeNewCb(data)

    def _afterNewCb(self, data):
        # Workaround to MAYA-65920: detect and avoid illegal callback sequence.
        if not self._beforeNewCbCalled:
            return

        self._beforeNewCbCalled = False

        self._afterOpenCb(data)
        
    def _afterOpenCb(self, data):
        # Observe stage changes, for all stages.  Return listener object can
        # optionally be used to call Revoke() to remove observation, but must
        # keep reference to it, otherwise its reference count is immediately
        # decremented, falls to zero, and no observation occurs.

        # Ideally, we would observe the data model only if there are observers,
        # to minimize cost of observation.  However, since observation is
        # frequent, we won't implement this for now.  PPT, 22-Dec-2017.
        for listener in self._stageListeners.itervalues():
            listener.Revoke()

        for stage in getStages():
            self._stageListeners[stage] = Tf.Notice.Register(
                Usd.Notice.ObjectsChanged, self.stageChanged, stage)

        # Set up our stage to AL_usdmaya_ProxyShape UFE path (and reverse)
        # mapping.  We do this with the following steps:
        # - get all proxyShape nodes in the scene.
        # - get their AL Python wrapper
        # - get their Dag paths
        # - convert the Dag paths to UFE paths.
        _stageToPath.clear()
        proxyShapeNames = usdRunTime.proxyShapeHandler.getAllNames()
        dagPaths = [utils.nameToDagPath(psn) for psn in proxyShapeNames]
        ufePaths = [dagPathToUfe(dagPath) for dagPath in dagPaths]

        for psn, ufePath in zip(proxyShapeNames, ufePaths):
            stage = usdRunTime.proxyShapeHandler.nameToStage(psn)
            _stageToPath[stage] = ufePath
            usdRunTime.pathToStage[ufePath] = stage

    def stageChanged(self, notice, sender):
        '''Call the stageChanged() methods on stage observers.

        Notice will be of type Usd.Notice.ObjectsChanged.  Sender is stage.'''
        
        # If the stage path has not been initialized yet, do nothing 
        if not stagePath(sender):
            return
        
        stage = notice.GetStage()
        for changedPath in notice.GetResyncedPaths():
            usdPrimPathStr = str(changedPath.GetPrimPath())
            ufePath = stagePath(sender) + ufe.PathSegment(
                usdPrimPathStr, 2, '/')
            prim = stage.GetPrimAtPath(changedPath)
            # Changed paths could be xformOps.
            # These are considered as invalid null prims
            if prim and not usdRunTime.inPathChange():
                if prim.IsActive():
                    notification = ufe.ObjectAdd(
                        ufe.Hierarchy.createItem(ufePath))
                    ufe.Scene.notifyObjectAdd(notification)
                else:
                    notification = ufe.ObjectPostDelete(
                        ufe.Hierarchy.createItem(ufePath))
                    ufe.Scene.notifyObjectDelete(notification)

        for changedPath in notice.GetChangedInfoOnlyPaths():
            usdPrimPathStr = str(changedPath.GetPrimPath())
            ufePath = stagePath(sender) + ufe.PathSegment(
                usdPrimPathStr, 2, '/')
            # We need to determine if the change is a Transform3d change.
            # We must at least pick up xformOp:translate, xformOp:rotateXYZ, 
            # and xformOp:scale.
            if changedPath.elementString.startswith('.xformOp:'):
                ufe.Transform3d.notify(ufePath)

def getStages():
    '''Get list of USD stages.'''
    return usdRunTime.proxyShapeHandler.getAllStages()

def initialize():
    # Only intended to be called by the plugin initialization, to
    # initialize the stage model.
    global _stagesSubject

    _stagesSubject = _StagesSubject()

def finalize():
    # Only intended to be called by the plugin finalization, to
    # finalize the stage model.
    global _stagesSubject

    _stagesSubject.finalize()
    del _stagesSubject
