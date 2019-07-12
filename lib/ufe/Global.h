// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================
#pragma once

#include "../base/api.h"

#include "maya/MStatus.h"

MAYAUSD_NS_DEF {
namespace ufe {

// Only intended to be called by the plugin initialization, to
// initialize the handlers and stage model.
MAYAUSD_CORE_PUBLIC
MStatus initialize();

//! Only intended to be called by the plugin finalization, to
//! finalize the handlers stage model.
MAYAUSD_CORE_PUBLIC
MStatus finalize();

} // namespace ufe
} // namespace MayaUsd
