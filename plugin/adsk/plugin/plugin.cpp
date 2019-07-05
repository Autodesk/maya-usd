// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "mayaUsd/api.h"

#include <maya/MFnPlugin.h>
#include <maya/MStatus.h>

MAYAUSD_PLUGIN_PUBLIC
MStatus initializePlugin(MObject obj)
{
   return MS::kSuccess;
}

MAYAUSD_PLUGIN_PUBLIC
MStatus uninitializePlugin(MObject obj)
{
   return MS::kSuccess;
}
