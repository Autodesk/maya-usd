// ===========================================================================
// Copyright 2019 Autodesk, Inc. All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license
// agreement provided at the time of installation or download, or which
// otherwise accompanies this software in either electronic or hard copy form.
// ===========================================================================

#include "render_param.h"

#include <maya/MProfiler.h>

PXR_NAMESPACE_OPEN_SCOPE

/*! \brief  Begin update before rendering of VP2 starts. 
*/
void HdVP2RenderParam::BeginUpdate(MSubSceneContainer& container, UsdTimeCode frame) {
    _container = &container;
    _frame = frame;
}

/*! \brief  End update & clear access to render item container which pass this point won't be valid.
*/
void HdVP2RenderParam::EndUpdate() {
    _container = nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
