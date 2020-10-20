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
#include <pxr/base/tf/token.h>

#include <maya/MApiNamespace.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class PxrMayaUsdPreviewSurfacePlugin
/// \brief Encapsulates plugin registration and deregistration of preview surface classes.
///
/// Preview surface support requires plugin registration of node classes, node
/// data, and draw support.  This class provides this service. Each client is expected
/// to provide a separate typeName and typeId to ensure proper plugin registration.
class PxrMayaUsdPreviewSurfacePlugin {
public:
    /// Initialize a UsdPreviewSurface dependency node named \p typeName with a unique \p typeId for
    /// the \p plugin using the registrant id \p registrantId for the render overrides.
    PXRUSDPREVIEWSURFACE_API
    static MStatus initialize(
        MFnPlugin&     plugin,
        const MString& typeName,
        MTypeId        typeId,
        const MString& registrantId);

    /// Deinitialize a UsdPreviewSurface dependency node named \p typeName with unique \p typeId for
    /// the \p plugin using the registrant id \p registrantId for the render overrides.
    PXRUSDPREVIEWSURFACE_API
    static MStatus finalize(
        MFnPlugin&     plugin,
        const MString& typeName,
        MTypeId        typeId,
        const MString& registrantId);

    PXRUSDPREVIEWSURFACE_API
    static MStatus registerFragments();

    PXRUSDPREVIEWSURFACE_API
    static MStatus deregisterFragments();

    PXRUSDPREVIEWSURFACE_API
    static void RegisterPreviewSurfaceReader(const MString& typeName);

    PXRUSDPREVIEWSURFACE_API
    static void RegisterPreviewSurfaceWriter(const MString& typeName);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
