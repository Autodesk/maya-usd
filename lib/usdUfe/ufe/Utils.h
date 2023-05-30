//
// Copyright 2021 Autodesk
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

#include <usdUfe/base/api.h>

#include <ufe/ufe.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <ufe/path.h>
#include <ufe/scene.h>

#include <string>

UFE_NS_DEF
{
    class PathSegment;
    class Selection;
}

namespace USDUFE_NS_DEF {

typedef PXR_NS::UsdPrim (*UfePathToPrimFn)(const Ufe::Path&);

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

//! Get the UFE path representing just the USD prim for the argument UFE path.
//! Any instance index component at the tail of the given path is removed from
//! the returned path.
USDUFE_PUBLIC
Ufe::Path stripInstanceIndexFromUfePath(const Ufe::Path& path);

//! Set the DCC specific ufe path to prim function.
//! It cannot be empty.
//! \exception std::invalid_argument If fn is empty.
USDUFE_PUBLIC
void setUfePathToPrimFn(UfePathToPrimFn fn);

//! Return the USD prim corresponding to the argument UFE path.
USDUFE_PUBLIC
PXR_NS::UsdPrim ufePathToPrim(const Ufe::Path& path);

//! Return the instance index corresponding to the argument UFE path if it
//! represents a point instance.
//! If the given path does not represent a point instance,
//! UsdImagingDelegate::ALL_INSTANCES (-1) will be returned.
//! If the optional argument prim pointer is non-null, the USD prim
//! corresponding to the argument UFE path is returned, as per ufePathToPrim().
USDUFE_PUBLIC
int ufePathToInstanceIndex(const Ufe::Path& path, PXR_NS::UsdPrim* prim = nullptr);

USDUFE_PUBLIC
bool isRootChild(const Ufe::Path& path);

//! Split the source name into a base name and a numerical suffix (set to
//! 1 if absent).  Increment the numerical suffix until name is unique.
USDUFE_PUBLIC
std::string uniqueName(const PXR_NS::TfToken::HashSet& existingNames, std::string srcName);

//! Return a unique child name.
USDUFE_PUBLIC
std::string uniqueChildName(const PXR_NS::UsdPrim& parent, const std::string& name);

//! Send notification for data model changes
template <class T>
void sendNotification(const Ufe::SceneItem::Ptr& item, const Ufe::Path& previousPath)
{
    T notification(item, previousPath);
#ifdef UFE_V2_FEATURES_AVAILABLE
    Ufe::Scene::instance().notify(notification);
#else
    Ufe::Scene::notifyObjectPathChange(notification);
#endif
}

//! Readability function to downcast a SceneItem::Ptr to a UsdSceneItem::Ptr.
inline UsdSceneItem::Ptr downcast(const Ufe::SceneItem::Ptr& item)
{
    return std::dynamic_pointer_cast<UsdSceneItem>(item);
}

//! Apply restriction rules on the given prim
USDUFE_PUBLIC
void applyCommandRestriction(
    const PXR_NS::UsdPrim& prim,
    const std::string&     commandName,
    bool                   allowStronger = false);

//! Apply restriction rules on the given prim
USDUFE_PUBLIC
bool applyCommandRestrictionNoThrow(
    const PXR_NS::UsdPrim& prim,
    const std::string&     commandName,
    bool                   allowStronger = false);

//! Check if the edit target in the stage is allowed to be changed.
//! \return True, if the edit target layer in the stage is allowed to be changed
USDUFE_PUBLIC
bool isEditTargetLayerModifiable(
    const PXR_NS::UsdStageWeakPtr stage,
    std::string*                  errMsg = nullptr);

} // namespace USDUFE_NS_DEF
