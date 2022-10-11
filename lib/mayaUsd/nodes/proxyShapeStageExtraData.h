//
// Copyright 2022 Autodesk
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
#ifndef MAYAUSD_PROXY_SHAPE_LOAD_RULES_H
#define MAYAUSD_PROXY_SHAPE_LOAD_RULES_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MObject.h>

namespace MAYAUSD_NS_DEF {

/// \class MayaUsdProxyShapeStageExtraData
/// \brief Encapsulates plugin registration and deregistration for the proxy shape extra data
/// handling.
///
/// USD proxy shape extra data are persisted on-disk in the proxy shape. We use a Maya callback
/// triggered before a scene is saved to copy the current proxy shape extra data from the stage
/// to the proxy shape.
///
/// The extra data saved this way currently are: payload load rules.

class MayaUsdProxyShapeStageExtraData
{
public:
    /// \brief initialise by registering the callbacks.
    MAYAUSD_CORE_PUBLIC
    static MStatus initialize();

    /// \brief finalize by deregistering the callbacks.
    MAYAUSD_CORE_PUBLIC
    static MStatus finalize();

    /// \brief add a proxy shape so that it will have its proxy shape extra data saved and loaded.
    MAYAUSD_CORE_PUBLIC
    static void addProxyShape(MayaUsdProxyShapeBase& proxyShape);

    /// \brief remove a proxy shape so that it will no longer have its proxy shape extra data saved
    /// and loaded.
    MAYAUSD_CORE_PUBLIC
    static void removeProxyShape(MayaUsdProxyShapeBase& proxyShape);

    /// \brief save all stage data of tracked proxy shapes.
    MAYAUSD_CORE_PUBLIC
    static void saveAllStageData();

    /// \brief save load rules of tracked proxy shapes.
    MAYAUSD_CORE_PUBLIC
    static void saveAllLoadRules();

    /// \brief save load rules of the tracked proxy shape corresponding to the given stage.
    MAYAUSD_CORE_PUBLIC
    static void saveLoadRules(const UsdStageRefPtr& stage);
};

} // namespace MAYAUSD_NS_DEF

#endif
