//
// Copyright 2019 Autodesk
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
#pragma once

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/prim.h>

#include <string>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ufe {

/*!
        Proxy shape abstraction, to support use of USD proxy shape with any plugin
        that has a proxy shape derived from MayaUsdProxyShapeBase.
 */
class MAYAUSD_CORE_PUBLIC ProxyShapeHandler
{
public:
    ProxyShapeHandler() = default;
    ~ProxyShapeHandler() = default;

    // Delete the copy/move constructors assignment operators.
    ProxyShapeHandler(const ProxyShapeHandler&) = delete;
    ProxyShapeHandler& operator=(const ProxyShapeHandler&) = delete;
    ProxyShapeHandler(ProxyShapeHandler&&) = delete;
    ProxyShapeHandler& operator=(ProxyShapeHandler&&) = delete;

    //! \return Type of the Maya shape node at the root of a USD hierarchy.
    static const std::string& gatewayNodeType();

    static std::vector<std::string> getAllNames();

    static PXR_NS::UsdStageWeakPtr dagPathToStage(const std::string& dagPath);

    static std::vector<PXR_NS::UsdStageRefPtr> getAllStages();

private:
    static const std::string fMayaUsdGatewayNodeType;

}; // ProxyShapeHandler

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
