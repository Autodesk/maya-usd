//
// Copyright 2024 Autodesk
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
#ifndef MAYAUSD_LAYERLOCKING_H
#define MAYAUSD_LAYERLOCKING_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

namespace MAYAUSD_NS_DEF {

// The lock state of a layer is stage-level data. As such, it is not saved
// within the layer (i.e. in the USD files that have been staged.) The reason
// behind this is that two stages could have different locked layers, a single
// layer could be locked in one stage and not locked in another stage. So, the
// locked state cannot be a layer-level data.
//
// Furthermore, stages in USD are not saved but are a pure run-time entity,
// part of the hosting application. It is thus the host responsibility to save
// stage-level state. So, we need to explicitly save the layer locked state.
//
// Additionally, there are multiple levels of locking defined for a layer
//     1- A layer is locked if layer's permission to edit is set to false
//     2- A layer is system locked by setting a layer's permission to edit
//        and a layer's permission to save both to false.
//
// However, the USD API for checking permissions is a result of the following
// conditions:
//
//     For permission to save to be True (SdfLayer::PermissionToSave()):
//
//     1- The layer must not be anonymous
//     2- The layer must not be muted
//     3- The layer must have write access to disk
//     4- The internal _permissionToSave must be True
//
//     For permission to Edit (SdfLayer::PermissionToEdit()):
//
//     1- The layer must not be muted
//     2- The internal _permissionToEdit must be True
//
// For this reason, the locked layer state needs to be managed inside Maya USD
// to avoid receiving false positives for PermissionToSave and PermissionToEdit.
//
// We therefore save the lock state of layers.
// OpenUSD forgets everything about layer permissions to save or edit as there are only applicable
// to sessions
//
// So we need to hold on to locked layers. We do this in a private global list
// of locked layers. That list gets cleared when a new Maya scene is created.

/*! \brief Adds a layer to the system lock list
 */
MAYAUSD_CORE_PUBLIC
void addSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer);

/*! \brief Removes a layer from the system lock list
 */
MAYAUSD_CORE_PUBLIC
void removeSystemLockedLayer(const PXR_NS::SdfLayerRefPtr& layer);

/*! \brief Checks if a layer is in the lock list.
 */
MAYAUSD_CORE_PUBLIC
bool isLayerSystemLocked(const PXR_NS::SdfLayerRefPtr& layer);

/*! \brief Clears the lock list
 */
MAYAUSD_CORE_PUBLIC
void forgetSystemLockedLayers();

} // namespace MAYAUSD_NS_DEF

#endif
