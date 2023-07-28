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
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/types.h>
#include <ufe/ufe.h>

#include <string>

UFE_NS_DEF
{
    class PathSegment;
    class Selection;
}

namespace USDUFE_NS_DEF {

// DCC specific accessor functions.
typedef PXR_NS::UsdStageWeakPtr (*StageAccessorFn)(const Ufe::Path&);
typedef Ufe::Path (*StagePathAccessorFn)(PXR_NS::UsdStageWeakPtr);
typedef PXR_NS::UsdPrim (*UfePathToPrimFn)(const Ufe::Path&);
typedef PXR_NS::UsdTimeCode (*TimeAccessorFn)(const Ufe::Path&);
typedef bool (*IsAttributeLockedFn)(const PXR_NS::UsdAttribute& attr, std::string* errMsg);
typedef void (*SaveStageLoadRulesFn)(const PXR_NS::UsdStageRefPtr&);

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------

//! Set the DCC specific stage accessor function.
//! It cannot be empty.
//! \exception std::invalid_argument If fn is empty.
USDUFE_PUBLIC
void setStageAccessorFn(StageAccessorFn fn);

//! Get USD stage corresponding to argument UFE path.
USDUFE_PUBLIC
PXR_NS::UsdStageWeakPtr getStage(const Ufe::Path& path);

//! Set the DCC specific stage path accessor function.
//! It cannot be empty.
//! \exception std::invalid_argument If fn is empty.
USDUFE_PUBLIC
void setStagePathAccessorFn(StagePathAccessorFn fn);

//! Return the ProxyShape node UFE path for the argument stage.
USDUFE_PUBLIC
Ufe::Path stagePath(PXR_NS::UsdStageWeakPtr stage);

//! Get the UFE path segment corresponding to the argument USD path.
//! If an instanceIndex is provided, the path segment for a point instance with
//! that USD path and index is returned.
USDUFE_PUBLIC
Ufe::PathSegment usdPathToUfePathSegment(
    const PXR_NS::SdfPath& usdPath,
    int                    instanceIndex = PXR_NS::UsdImagingDelegate::ALL_INSTANCES);

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

//! Set the DCC specific time accessor function.
//! It cannot be empty.
//! \excpection std::invalid_argument if fn is empty.
USDUFE_PUBLIC
void setTimeAccessorFn(TimeAccessorFn fn);

//! Get the time along the argument path.
USDUFE_PUBLIC
PXR_NS::UsdTimeCode getTime(const Ufe::Path& path);

//! Set the DCC specific USD attribute is locked test function.
//! Use of this function is optional, if one is not supplied then
//! default value (false) will be returned by accessor function.
USDUFE_PUBLIC
void setIsAttributeLockedFn(IsAttributeLockedFn fn);

//! Return whether the input USD attribute is locked and therefore cannot
//! be edited.
//! \return True if the USD attributed is locked, otherwise false (default).
USDUFE_PUBLIC
bool isAttributedLocked(const PXR_NS::UsdAttribute& attr, std::string* errMsg = nullptr);

//! Set the DCC specific USD save load rules function.
//! Use of this function is optional, if one is supplied then it
//! will be called when loading/unloading a payload.
//! Save the load rules so that switching the stage settings will
//! be able to preserve the load rules.
USDUFE_PUBLIC
void setSaveStageLoadRulesFn(SaveStageLoadRulesFn fn);

//! If an optional save load rules function was set, call it.
//! If a stage is provided then only that stage is used, otherwise
//! all stages will be used.
void saveStageLoadRules(const PXR_NS::UsdStageRefPtr& stage);

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
    Ufe::Scene::instance().notify(notification);
}

//! Readability function to downcast a SceneItem::Ptr to a UsdSceneItem::Ptr.
inline UsdSceneItem::Ptr downcast(const Ufe::SceneItem::Ptr& item)
{
    return std::dynamic_pointer_cast<UsdSceneItem>(item);
}

//! Filter a source selection by removing descendants of filterPath.
USDUFE_PUBLIC
Ufe::Selection removeDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath);

//! Re-build a source selection by copying scene items that are not descendants
//! of filterPath to the destination, and re-creating the others into the
//! destination using the source scene item path.
USDUFE_PUBLIC
Ufe::Selection recreateDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath);

//------------------------------------------------------------------------------
// Verify edit restrictions.
//------------------------------------------------------------------------------

//! Check if an attribute value is allowed to be changed.
//! \return True, if the attribute value is allowed to be edited in the stage's local Layer Stack.
USDUFE_PUBLIC
bool isAttributeEditAllowed(const PXR_NS::UsdAttribute& attr, std::string* errMsg = nullptr);

USDUFE_PUBLIC
bool isAttributeEditAllowed(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& attrName,
    std::string*           errMsg);

USDUFE_PUBLIC
bool isAttributeEditAllowed(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName);

//! Enforce if an attribute value is allowed to be changed. Throw an exceptio if not allowed.
USDUFE_PUBLIC
void enforceAttributeEditAllowed(const PXR_NS::UsdAttribute& attr);

USDUFE_PUBLIC
void enforceAttributeEditAllowed(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName);

//! Check if a prim metadata is allowed to be changed.
//! Can check a specific key in a metadata dictionary, optionally, if keyPaty is not empty.
//! \return True, if the metadata value is allowed to be edited in the stage's local Layer Stack.
USDUFE_PUBLIC
bool isPrimMetadataEditAllowed(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& metadataName,
    const PXR_NS::TfToken& keyPath,
    std::string*           errMsg);

//! Check if a property metadata is allowed to be changed.
//! Can check a specific key in a metadata dictionary, optionally, if keyPaty is not empty.
//! \return True, if the metadata value is allowed to be edited in the stage's local Layer Stack.
USDUFE_PUBLIC
bool isPropertyMetadataEditAllowed(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& propName,
    const PXR_NS::TfToken& metadataName,
    const PXR_NS::TfToken& keyPath,
    std::string*           errMsg);

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

//! Combine two UFE bounding boxes.
USDUFE_PUBLIC
Ufe::BBox3d combineUfeBBox(const Ufe::BBox3d& ufeBBox1, const Ufe::BBox3d& ufeBBox2);

} // namespace USDUFE_NS_DEF
