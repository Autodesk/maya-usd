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

try:
    import ufe
except:
    print "Unable to load ufe - Make sure the ufe path is added to PYTHONPATH"
    raise
    
    
from ufeScripts import usdRunTime
from ufeScripts import usdSubject
import sys
import maya.api.OpenMaya as om

def maya_useNewAPI():
    """
    The presence of this function tells Maya that the plugin produces, and
    expects to be passed, objects created using the Maya Python API 2.0.
    """
    pass

def initializePlugin(obj):
    plugin = om.MFnPlugin(obj, "Autodesk", "1.0", "Any")
    usdRunTime.initialize()
    usdSubject.initialize()

def uninitializePlugin(obj):
    usdRunTime.finalize()
    usdSubject.finalize()
    del sys.modules['usdRunTime']
    del sys.modules['usdSubject']
    

