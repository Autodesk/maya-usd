//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef PXRUSDMAYA_PREVIEW_SURFACE_PLUGIN_H
#define PXRUSDMAYA_PREVIEW_SURFACE_PLUGIN_H

#include "api.h"

#include <pxr/pxr.h>

#include <maya/MApiNamespace.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PxrMayaUsdPreviewSurfacePlugin
/// \brief Encapsulates plugin registration and deregistration of preview surface classes.
///
/// Preview surface support requires plugin registration of node classes, node
/// data, and draw support.  This class provides this service, including if
/// multiple plugins that use preveiw surface are loaded: using reference
/// counting, only the first registration and the last deregistration will
/// be performed.  Note that because of Maya architecture requirements,
/// deregistration will only be done if the deregistering plugin is the same as
/// the registering plugin.  Otherwise, a warning is shown.

class PxrMayaUsdPreviewSurfacePlugin {
public:
    PXRUSDPREVIEWSURFACE_API
    static MStatus initialize(MFnPlugin&);

    PXRUSDPREVIEWSURFACE_API
    static MStatus finalize(MFnPlugin&);

    static MStatus registerFragments();

    static MStatus deregisterFragments();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
