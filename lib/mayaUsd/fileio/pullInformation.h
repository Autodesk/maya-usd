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
#include <pxr/usd/usd/prim.h>

#include <maya/MDagPath.h>
#include <ufe/sceneItem.h>

UFE_NS_DEF { class Path; }

namespace MAYAUSD_NS_DEF {

MAYAUSD_CORE_PUBLIC
bool writePullInformation(const Ufe::Path& ufePulledPath, const MDagPath& editedAsMayaRoot);

MAYAUSD_CORE_PUBLIC
bool readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr);
MAYAUSD_CORE_PUBLIC
bool readPullInformation(const PXR_NS::UsdPrim& prim, Ufe::SceneItem::Ptr& dagPathItem);
MAYAUSD_CORE_PUBLIC
bool readPullInformation(const Ufe::Path& ufePath, MDagPath& dagPath);
MAYAUSD_CORE_PUBLIC
bool readPullInformation(const MDagPath& dagpath, Ufe::Path& ufePath);

MAYAUSD_CORE_PUBLIC
bool writePulledPrimMetadata(const Ufe::Path& ufePulledPath, const MDagPath& editedRoot);
MAYAUSD_CORE_PUBLIC
bool writePulledPrimMetadata(PXR_NS::UsdPrim& pulledPrim, const MDagPath& editedRoot);

MAYAUSD_CORE_PUBLIC
void removePulledPrimMetadata(const Ufe::Path& ufePulledPath);
MAYAUSD_CORE_PUBLIC
void removePulledPrimMetadata(const PXR_NS::UsdStagePtr& stage, PXR_NS::UsdPrim& prim);

MAYAUSD_CORE_PUBLIC
bool addExcludeFromRendering(const Ufe::Path& ufePulledPath);

MAYAUSD_CORE_PUBLIC
bool removeExcludeFromRendering(const Ufe::Path& ufePulledPath);

}

#endif
