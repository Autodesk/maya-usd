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
#ifndef MAYAUSD_LAYERMUTING_H
#define MAYAUSD_LAYERMUTING_H

#include <mayaUsd/base/api.h>

#include <pxr/usd/usd/stage.h>

#include <maya/MObject.h>
#include <maya/MString.h>

namespace MAYAUSD_NS_DEF {

// The muted state of a layer is stage-level data. As such, it is not saved
// within the layer (i.e. in the USD files that have been staged.) The reason
// behind this is that two stages could have different muted layers, a single
// layer could be muted in one stage and not muted in another stage. So, the
// muted state cannot be a layer-level data.
//
// Furthermore, stages in USD are not saved but are apure-run-time entity,
// part of the hosting application. It is thus the host responsibility to save
// stage-level state. So, we need to explicitly save the layer muted state.
//
// We thus saved the muted state of layers in the proxy shape as a dynamic
// attribute.

/*! \brief convert the stage layers muting to a text format.
 */
MAYAUSD_CORE_PUBLIC
MString convertLayerMutingToText(const PXR_NS::UsdStage& stage);

/*! \brief set the stage layers muting from a text format.
 */
MAYAUSD_CORE_PUBLIC
void setLayerMutingFromText(PXR_NS::UsdStage& stage, const MString& text);

/*! \brief verify if there is a dynamic attribute on the object for layers muting.
 */
MAYAUSD_CORE_PUBLIC
bool hasLayerMutingAttribute(const MObject& proxyShape);

/*! \brief copy the stage layers muting in a dynamic attribute on the object.
 */
MAYAUSD_CORE_PUBLIC
MStatus copyLayerMutingToAttribute(const PXR_NS::UsdStage& stage, MObject& proxyShape);

/*! \brief set the stage layers muting from data in a dynamic attribute on the object.
 */
MAYAUSD_CORE_PUBLIC
MStatus copyLayerMutingFromAttribute(const MObject& proxyShape, PXR_NS::UsdStage& stage);

} // namespace MAYAUSD_NS_DEF

#endif
