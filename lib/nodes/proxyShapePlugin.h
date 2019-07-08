//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXRUSDMAYA_PROXY_SHAPE_PLUGIN_H
#define PXRUSDMAYA_PROXY_SHAPE_PLUGIN_H

/// \file usdMaya/proxyShapePlugin.h

#include "../base/api.h"

#include "pxr/pxr.h"

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
