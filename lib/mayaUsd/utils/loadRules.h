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
#ifndef MAYAUSD_LOADRULES_H
#define MAYAUSD_LOADRULES_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MApiNamespace.h>

namespace MAYAUSD_NS_DEF {

/*! \brief verify if there is a dynamic attribute on the object for load rules.
 */
MAYAUSD_CORE_PUBLIC
bool hasLoadRulesAttribute(const PXR_NS::MayaUsdProxyShapeBase& proxyShape);

/*! \brief copy the stage load rules in a dynamic attribute on the object.
 */
MAYAUSD_CORE_PUBLIC
MStatus
copyLoadRulesToAttribute(const PXR_NS::UsdStage& stage, PXR_NS::MayaUsdProxyShapeBase& proxyShape);

/*! \brief set the stage load rules from data in a dynamic attribute on the object.
 */
MAYAUSD_CORE_PUBLIC
MStatus copyLoadRulesFromAttribute(
    const PXR_NS::MayaUsdProxyShapeBase& proxyShape,
    PXR_NS::UsdStage&                    stage);

} // namespace MAYAUSD_NS_DEF

#endif
