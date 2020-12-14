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
#ifndef PXRUSDMAYA_PROXY_SHAPE_PLUGIN_H
#define PXRUSDMAYA_PROXY_SHAPE_PLUGIN_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <maya/MApiNamespace.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class MayaUsdProxyShapePlugin
/// \brief Encapsulates plugin registration and deregistration of proxy shape classes.
///
/// Proxy shape support requires plugin registration of node classes, node
/// data, and draw support.  This class provides this service, including if
/// multiple plugins that use proxy shapes are loaded: using reference
/// counting, only the first registration and the last deregistration will
/// be performed.  Note that because of Maya architecture requirements,
/// deregistration will only be done if the deregistering plugin is the same as
/// the registering plugin.  Otherwise, a warning is shown.

class MayaUsdProxyShapePlugin
{
public:
    MAYAUSD_CORE_PUBLIC
    static MStatus initialize(MFnPlugin&);

    MAYAUSD_CORE_PUBLIC
    static MStatus finalize(MFnPlugin&);

    MAYAUSD_CORE_PUBLIC
    static const MString* getProxyShapeClassification();

    MAYAUSD_CORE_PUBLIC
    static bool useVP2_NativeUSD_Rendering();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
