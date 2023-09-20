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
#ifndef PXRUSDMAYA_MAYAPRIMUPDATER_MANAGER_IMPL_H
#define PXRUSDMAYA_MAYAPRIMUPDATER_MANAGER_IMPL_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/editTarget.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>
#include <ufe/sceneItem.h>

UFE_NS_DEF { class Path; }

namespace MAYAUSD_NS_DEF {

////////////////////////////////////////////////////////////////////////////
//
// Helper functions to write, read and remove edited-as-Maya (pull)
// information set on the Maya DAG node.

/// @brief Write on the Maya node the information necessary later-on to merge
///        the USD prim that is edited as Maya.
MAYAUSD_CORE_PUBLIC
bool writePullInformation(const Ufe::Path& ufePulledPath, const MDagPath& editedAsMayaRoot);

/// @brief Read on the Maya node the information necessary to merge the USD prim
///        that is edited as Maya.
MAYAUSD_CORE_PUBLIC
bool readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr);

MAYAUSD_CORE_PUBLIC
bool readPullInformation(const PXR_NS::UsdPrim& prim, Ufe::SceneItem::Ptr& dagPathItem);

MAYAUSD_CORE_PUBLIC
bool readPullInformation(const Ufe::Path& ufePath, MDagPath& dagPath);

MAYAUSD_CORE_PUBLIC
bool readPullInformation(const MDagPath& dagpath, Ufe::Path& ufePath);

////////////////////////////////////////////////////////////////////////////
//
// Helper functions to write, read and remove edited-as-Maya (pull)
// information set on the edited USD prim.

/// @brief Write on the USD prim the information necessary later-on to merge
///        the USD prim that is edited as Maya.
MAYAUSD_CORE_PUBLIC
bool writePulledPrimMetadata(const Ufe::Path& ufePulledPath, const MDagPath& editedRoot);

/// @brief Write on the Maya node the information necessary later-on to merge
///        the USD prim that is edited as Maya, in the given edit target.
MAYAUSD_CORE_PUBLIC
bool writePulledPrimMetadata(
    const Ufe::Path&             ufePulledPath,
    const MDagPath&              editedRoot,
    const PXR_NS::UsdEditTarget& editTarget);

/// @brief Write on the USD prim the information necessary later-on to merge
///        the USD prim that is edited as Maya.
MAYAUSD_CORE_PUBLIC
bool writePulledPrimMetadata(PXR_NS::UsdPrim& pulledPrim, const MDagPath& editedRoot);

/// @brief Write on the Maya node the information necessary later-on to merge
///        the USD prim that is edited as Maya, in the given edit target.
MAYAUSD_CORE_PUBLIC
bool writePulledPrimMetadata(
    PXR_NS::UsdPrim&             pulledPrim,
    const MDagPath&              editedAsMayaRoot,
    const PXR_NS::UsdEditTarget& editTarget);

/// @brief Remove from the USD prim the information necessary to merge the USD prim
///        that was edited as Maya.
MAYAUSD_CORE_PUBLIC
void removePulledPrimMetadata(const Ufe::Path& ufePulledPath);

/// @brief Remove from the USD prim the information necessary to merge the USD prim
///        that was edited as Maya, .
MAYAUSD_CORE_PUBLIC
void removePulledPrimMetadata(
    const Ufe::Path&             ufePulledPath,
    const PXR_NS::UsdEditTarget& editTarget);

/// @brief Remove from the USD prim the information necessary to merge the USD prim
///        that was edited as Maya.
MAYAUSD_CORE_PUBLIC
void removePulledPrimMetadata(const PXR_NS::UsdStagePtr& stage, PXR_NS::UsdPrim& prim);

/// @brief Remove from the USD prim the information necessary to merge the USD prim
///        that was edited as Maya, in the given edit target.
MAYAUSD_CORE_PUBLIC
void removePulledPrimMetadata(
    const PXR_NS::UsdStagePtr&   stage,
    PXR_NS::UsdPrim&             prim,
    const PXR_NS::UsdEditTarget& editTarget);

////////////////////////////////////////////////////////////////////////////
//
// Helper functions to hide and show the edited prim.

/// @brief Hide the USD prim that is edited as Maya.
///        This is done so that the USD prim and edited Maya data are not superposed
///        in the viewport.
MAYAUSD_CORE_PUBLIC
bool addExcludeFromRendering(const Ufe::Path& ufePulledPath);

MAYAUSD_CORE_PUBLIC
bool addExcludeFromRendering(
    const Ufe::Path&             ufePulledPath,
    const PXR_NS::UsdEditTarget& editTarget);

/// @brief Show again the USD prim that was edited as Maya.
///        This is done once the Maya data is meged into USD and removed from the scene.
MAYAUSD_CORE_PUBLIC
bool removeExcludeFromRendering(const Ufe::Path& ufePulledPath);

MAYAUSD_CORE_PUBLIC
bool removeExcludeFromRendering(
    const Ufe::Path&             ufePulledPath,
    const PXR_NS::UsdEditTarget& editTarget);

////////////////////////////////////////////////////////////////////////////
//
// Helper functions to check if a prim is already edited-as-Maya.

/// @brief Verify if the edited as Maya nodes corresponding to the given prim is orphaned.
MAYAUSD_CORE_PUBLIC
bool isEditedAsMayaOrphaned(const PXR_NS::UsdPrim& prim);

MAYAUSD_CORE_PUBLIC
bool isEditedAsMayaOrphaned(const Ufe::Path& editedUsdPrim);

} // namespace MAYAUSD_NS_DEF

#endif
