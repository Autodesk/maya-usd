//
// Copyright 2023 Autodesk
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
#ifndef MAYAUSD_TARGETLAYER_H
#define MAYAUSD_TARGETLAYER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MApiNamespace.h>

namespace MAYAUSD_NS_DEF {

// The current target layer is stage-level data. As such, it is not saved
// within the layer (i.e. in the USD files that have been staged.) The reason
// behind this is that two stages could have different target layers. So, the
// target layer cannot be a layer-level data.
//
// Furthermore, stages in USD are not saved but are a pure run-time entity,
// part of the hosting application. It is thus the host responsibility to save
// stage-level state. So, we need to explicitly save the target layer.
//
// We thus save the target layer in the proxy shape as an attribute.

/*! \brief convert the stage target layer to a text format.
 */
MAYAUSD_CORE_PUBLIC
MString convertTargetLayerToText(const PXR_NS::UsdStage& stage);

/*! \brief get the target layer from a text format if it exists on the given stage.
 */
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerHandle getTargetLayerFromText(PXR_NS::UsdStage& stage, const MString& text);

/*! \brief set the stage target layer from a text format.
 */
MAYAUSD_CORE_PUBLIC
bool setTargetLayerFromText(PXR_NS::UsdStage& stage, const MString& text);

/*! \brief get the target layer ID from data in the corresponding attribute of the proxy shape.
 */
MAYAUSD_CORE_PUBLIC
MString getTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape);

/*! \brief get the target layer from data in the corresponding attribute of the proxy shape if it
 * exists on the given stage.
 */
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerHandle
getTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage);

/*! \brief copy the stage target layer in the corresponding attribute of the proxy shape.
 */
MAYAUSD_CORE_PUBLIC
MStatus
copyTargetLayerToAttribute(const PXR_NS::UsdStage& stage, MayaUsdProxyShapeBase& proxyShape);

/*! \brief set the stage target layer from data in the corresponding attribute of the proxy shape.
 */
MAYAUSD_CORE_PUBLIC
MStatus
copyTargetLayerFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage);

/*! \brief get the edit target from data in the corresponding attribute of the proxy shape if it
 * exists on the given stage, the edit target layer could be a local layer or non local layer.
 */
MAYAUSD_CORE_PUBLIC
PXR_NS::UsdEditTarget
getEditTargetFromAttribute(const MayaUsdProxyShapeBase& proxyShape, PXR_NS::UsdStage& stage);

} // namespace MAYAUSD_NS_DEF

#endif
