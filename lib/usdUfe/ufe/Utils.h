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
#ifndef USDUFE_UFE_UTILS_H
#define USDUFE_UFE_UTILS_H

#include <usdUfe/base/api.h>
#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdr/shaderNode.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/property.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <ufe/attribute.h>
#include <ufe/hierarchy.h>
#include <ufe/path.h>
#include <ufe/scene.h>
#include <ufe/types.h>
#include <ufe/ufe.h>

#ifdef UFE_V3_FEATURES_AVAILABLE
#include <pxr/base/vt/value.h>

#include <ufe/value.h>
#endif // UFE_V3_FEATURES_AVAILABLE

#include <string>

UFE_NS_DEF
{
    class Attribute;
    class AttributeInfo;
    class PathSegment;
    class Selection;
}

namespace USDUFE_NS_DEF {

class UsdAttribute;
class UsdProperty;
class UsdUndoableItem;

// DCC specific accessor functions.
typedef PXR_NS::UsdStageWeakPtr (*StageAccessorFn)(const Ufe::Path&);
typedef Ufe::Path (*StagePathAccessorFn)(PXR_NS::UsdStageWeakPtr);
typedef PXR_NS::UsdPrim (*UfePathToPrimFn)(const Ufe::Path&);
typedef PXR_NS::UsdTimeCode (*TimeAccessorFn)(const Ufe::Path&);
typedef bool (*IsAttributeLockedFn)(const PXR_NS::UsdProperty& attr, std::string* errMsg);
typedef void (*SaveStageLoadRulesFn)(const PXR_NS::UsdStageRefPtr&);
typedef bool (*IsRootChildFn)(const Ufe::Path& path);
typedef std::string (*UniqueChildNameFn)(const PXR_NS::UsdPrim& usdParent, const std::string& name, const std::string* excludeName);
typedef void (*DisplayMessageFn)(const std::string& msg);
typedef void (*WaitCursorFn)();
typedef std::string (*DefaultMaterialScopeNameFn)();
typedef void (
    *ExtractTRSFn)(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s);
typedef const char* (*Transform3dMatrixOpNameFn)();

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

USDUFE_PUBLIC
UsdSceneItem::Ptr
createSiblingSceneItem(const Ufe::Path& ufeSrcPath, const std::string& siblingName);

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
//! a default test function will be used.
USDUFE_PUBLIC
void setIsAttributeLockedFn(IsAttributeLockedFn fn);

//! Return whether the input USD property (attribute or relationship) is locked and therefore cannot
//! be edited.
//! \return True if the USD property is locked, otherwise false
//! (default).
USDUFE_PUBLIC
bool isAttributedLocked(const PXR_NS::UsdProperty& attr, std::string* errMsg = nullptr);

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

//! Set the DCC specific "isRootChild" test function.
//! Use of this function is optional, if one is not supplied then
//! a default implementation of isRootChild is used.
USDUFE_PUBLIC
void setIsRootChildFn(IsRootChildFn fn);

//! Returns true if the path corresponds to an item at the root of a runtime.
//! Implementation can be set by the DCC.
USDUFE_PUBLIC
bool isRootChild(const Ufe::Path& path);

//! Default isRootChild() implementation. Assumes 2 segments. Will report a root child
//! if the second segment has a single component.
USDUFE_PUBLIC
bool isRootChildDefault(const Ufe::Path& path);

//! Split the input source name <p srcName> into a base name <p base> and a
//! numerical suffix (if present) <p suffix>.
//! Returns true when numerical suffix was found, otherwise false.
USDUFE_PUBLIC
bool splitNumericalSuffix(const std::string srcName, std::string& base, std::string& suffix);

//! Split the source name into a base name and a numerical suffix (set to
//! 1 if absent).  Increment the numerical suffix until name is unique.
USDUFE_PUBLIC
std::string uniqueName(const PXR_NS::TfToken::HashSet& existingNames, std::string srcName);

//! Set the DCC specific "uniqueChildName" function.
//! Use of this function is optional, if one is not supplied then
//! a default implementation of uniqueChildName is used.
USDUFE_PUBLIC
void setUniqueChildNameFn(UniqueChildNameFn fn);

//! Return a unique child name.
USDUFE_PUBLIC
std::string uniqueChildName(const PXR_NS::UsdPrim& usdParent, const std::string& name);

//! Return a unique child name.
//! \param excludeName Optional name to exclude from uniqueness check (e.g., when renaming, to not check against its old value). If nullptr, no name is excluded.
USDUFE_PUBLIC
std::string uniqueChildName(const PXR_NS::UsdPrim& usdParent, const std::string& name, const std::string* excludeName);

//! Return a relatively unique prim name.
//! That is, make some effort so that the name is unique relative to other prims
//! "around" it, like ancestors and some descendants.
USDUFE_PUBLIC
std::string relativelyUniqueName(const PXR_NS::UsdPrim& usdParent, const std::string& name);

//! Default uniqueChildName() implementation. Uses all the prim's children.
USDUFE_PUBLIC
std::string uniqueChildNameDefault(const PXR_NS::UsdPrim& parent, const std::string& name);

//! Default uniqueChildName() implementation. Uses all the prim's children.
USDUFE_PUBLIC
std::string uniqueChildNameDefault(
    const PXR_NS::UsdPrim& parent,
    const std::string&     name,
    const std::string*     excludeName);

//! Return a unique SdfPath by looking at existing siblings under the path's parent.
USDUFE_PUBLIC
PXR_NS::SdfPath uniqueChildPath(const PXR_NS::UsdStage& stage, const PXR_NS::SdfPath& path);

USDUFE_PUBLIC
Ufe::Path appendToUsdPath(const Ufe::Path& path, const std::string& name);

//! Returns true if \p item is a materials scope.
USDUFE_PUBLIC
bool isMaterialsScope(const Ufe::SceneItem::Ptr& item);

//! Support message types.
enum class MessageType
{
    kInfo,    // Displays an information message, default = TF_STATUS
    kWarning, // Displays a warning message, default = TF_WARN
    kError,   // Displays a error message, default = TF_RUNTIME_ERROR

    nbTypes
};

//! Set the DCC specific "displayMessage" functions.
//! Use of these functions is optional, if not supplied then default
//! TF_ message functions from USD will be used.
USDUFE_PUBLIC
void setDisplayMessageFn(const DisplayMessageFn fns[static_cast<int>(MessageType::nbTypes)]);

//! Displays a message of the given type using a DCC message function (if provided)
//! otherwise displays using a USD default message function (for each type)..
USDUFE_PUBLIC
void displayMessage(MessageType type, const std::string& msg);

USDUFE_PUBLIC
Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::UsdProperty& usdProp);

USDUFE_PUBLIC
Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::UsdAttribute& usdAttr);

USDUFE_PUBLIC
Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::SdrShaderPropertyConstPtr& shaderProperty);

USDUFE_PUBLIC
PXR_NS::SdfValueTypeName ufeTypeToUsd(const Ufe::Attribute::Type ufeType);

USDUFE_PUBLIC
UsdAttribute* usdAttrFromUfeAttr(const Ufe::Attribute::Ptr& attr);

#ifdef UFE_V4_FEATURES_AVAILABLE
USDUFE_PUBLIC
Ufe::Attribute::Ptr attrFromUfeAttrInfo(const Ufe::AttributeInfo& attrInfo);
#endif // UFE_V4_FEATURES_AVAILABLE

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

//! Copy the argument matrix into the return matrix.
USDUFE_PUBLIC
Ufe::Matrix4d toUfe(const PXR_NS::GfMatrix4d& src);

//! Copy the argument matrix into the return matrix.
USDUFE_PUBLIC
PXR_NS::GfMatrix4d toUsd(const Ufe::Matrix4d& src);

//! Copy the argument vector into the return vector.
USDUFE_PUBLIC
Ufe::Vector3d toUfe(const PXR_NS::GfVec3d& src);

//! Copy the argument vector into the return vector.
USDUFE_PUBLIC
PXR_NS::GfVec3d toUsd(const Ufe::Vector3d& src);

//! Filter a source selection by removing descendants of filterPath.
USDUFE_PUBLIC
Ufe::Selection removeDescendants(
    const Ufe::Selection& src,
    const Ufe::Path&      filterPath,
    bool*                 itemRemoved = nullptr);

//! Re-build a source selection by copying scene items that are not descendants
//! of filterPath to the destination, and re-creating the others into the
//! destination using the source scene item path.
USDUFE_PUBLIC
Ufe::Selection recreateDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath);

#ifdef UFE_V3_FEATURES_AVAILABLE
//! Converts a UFE Value to a VtValue
USDUFE_PUBLIC PXR_NS::VtValue ufeValueToVtValue(const Ufe::Value& ufeValue);

//! Converts a VtValue to a UFE Value
USDUFE_PUBLIC Ufe::Value vtValueToUfeValue(const PXR_NS::VtValue& vtValue);
#endif // UFE_V3_FEATURES_AVAILABLE

//! Returns the Sdr shader node for the given SceneItem. If the
//! definition associated with the scene item's type is not found, a
//! nullptr is returned.
USDUFE_PUBLIC
PXR_NS::SdrShaderNodeConstPtr usdShaderNodeFromSceneItem(const Ufe::SceneItem::Ptr& item);

//------------------------------------------------------------------------------
// Verify edit restrictions.
//------------------------------------------------------------------------------

//! Check if an property (attribute or relationship) value is allowed to be changed.
//! \return True, if the property value is allowed to be edited in the stage's local Layer Stack.
USDUFE_PUBLIC
bool isAttributeEditAllowed(const PXR_NS::UsdProperty& attr, std::string* errMsg = nullptr);

USDUFE_PUBLIC
bool isAttributeEditAllowed(
    const PXR_NS::UsdPrim& prim,
    const PXR_NS::TfToken& attrName,
    std::string*           errMsg);

USDUFE_PUBLIC
bool isAttributeEditAllowed(const PXR_NS::UsdPrim& prim, const PXR_NS::TfToken& attrName);

//! Enforce if an property (attribute or relationship) value is allowed to be changed. Throw an
//! exception if not allowed.
USDUFE_PUBLIC
void enforceAttributeEditAllowed(const PXR_NS::UsdProperty& attr);

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

//! Apply restriction rules for root layer metadata on the given prim
USDUFE_PUBLIC
void applyRootLayerMetadataRestriction(const PXR_NS::UsdPrim& prim, const std::string& commandName);

//! Apply restriction rules for root layer metadata on the given stage
USDUFE_PUBLIC
void applyRootLayerMetadataRestriction(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            commandName);

//! Check if any layers in the stage is allowed to be changed.
//! \return True, if at least one layer in the stage is allowed to be changed
USDUFE_PUBLIC
bool isAnyLayerModifiable(const PXR_NS::UsdStageWeakPtr stage, std::string* errMsg = nullptr);

//! Check if the edit target in the stage is allowed to be changed.
//! \return True, if the edit target layer in the stage is allowed to be changed
USDUFE_PUBLIC
bool isEditTargetLayerModifiable(
    const PXR_NS::UsdStageWeakPtr stage,
    std::string*                  errMsg = nullptr);

//! Check if a vector of paths could be added/removed on a given relationship.
//! \param relationship The relationship to be edited.
//! \param targetsToAdd The optional paths to be added to the relationship. If
//!        not nullptr, the entries will be adjusted to only keep the paths that
//!        are allowed to be added to the relationship.
//! \param targetsToRemove The optional paths to be removed from the
//!        relationship. If not nullptr, the entries will be adjusted to only
//!        keep the paths that are allowed to be removed from the relationship.
//! \param errMsg The optional error message to be filled in case of any target
//!        not being able to be added/removed.
//! \return True, if one ore more of the targets given are allowed to be
//!        added/removed to the relationship in the stage's local Layer Stack.
USDUFE_PUBLIC
bool isRelationshipEditAllowed(
    const PXR_NS::UsdRelationship& relationship,
    PXR_NS::SdfPathVector*         targetsToAdd = nullptr,
    PXR_NS::SdfPathVector*         targetsToRemove = nullptr,
    std::string*                   errMsg = nullptr);

//! Combine two UFE bounding boxes.
USDUFE_PUBLIC
Ufe::BBox3d combineUfeBBox(const Ufe::BBox3d& ufeBBox1, const Ufe::BBox3d& ufeBBox2);

//! Set both the start and stop wait cursor functions.
USDUFE_PUBLIC
void setWaitCursorFns(WaitCursorFn startFn, WaitCursorFn stopFn);

//! Start the wait cursor. Can be called recursively.
USDUFE_PUBLIC
void startWaitCursor();

//! Stop the wait cursor. Can be called recursively.
USDUFE_PUBLIC
void stopWaitCursor();

//! Start and stop the wait cursor in the constructor and destructor.
struct USDUFE_PUBLIC WaitCursor
{
    //! Show the wait cursor if the showCursor flag is true.
    WaitCursor(bool showCursor = true)
        : _showCursor(showCursor)
    {
        if (_showCursor)
            startWaitCursor();
    }

    //! Stop the wait cursor if the showCursor flag is true.
    ~WaitCursor()
    {
        if (_showCursor)
            stopWaitCursor();
    }

    USDUFE_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(WaitCursor);

    const bool _showCursor;
};

//! Set the DCC specific default material scope name function.
//! Use of this function is optional, if one is not supplied then
//! a default name will be used.

USDUFE_PUBLIC
void setDefaultMaterialScopeNameFn(DefaultMaterialScopeNameFn fn);

//! Returns the default material scope name.
USDUFE_PUBLIC
std::string defaultMaterialScopeName();

// Search the parent Material of the item, comparing the type name.
// In the case item is a material, return item itself.
USDUFE_PUBLIC
UsdSceneItem::Ptr getParentMaterial(const UsdSceneItem::Ptr& item);

//! Set the DCC specific extract TRS (Translate/Rotate/Scale)
//! function "extractTRS".
//! Use of this function is optional, if one is not supplied then
//! a default one will be used (that uses USD API to extract).
USDUFE_PUBLIC
void setExtractTRSFn(ExtractTRSFn fn);

//! Extract the TRS (Translate/Rotate/Scale) from the input matrix.
USDUFE_PUBLIC
void extractTRS(const Ufe::Matrix4d& m, Ufe::Vector3d* t, Ufe::Vector3d* r, Ufe::Vector3d* s);

//! Set the DCC function that is used to obtain a transform3d matrix
//! op name. Use of this function is optional. If one is not supplied
//! then no special transform3d matrix op name will be used.
USDUFE_PUBLIC
void setTransform3dMatrixOpNameFn(Transform3dMatrixOpNameFn fn);

//! Return the transform3d matrix op name to be used.
//! By default nullptr is returned.
USDUFE_PUBLIC
const char* getTransform3dMatrixOpName();

//! Verify if the metadata group name has the session prefix and provide the prefix-less group name.
//! \note the prefix-less name is only filled when returning true.
USDUFE_PUBLIC
bool isSessionLayerGroupMetadata(const std::string& groupName, std::string* adjustedGroupName);

//! Remove data left behind in the session layer for the given prim path in the given stage
//! and store the undos as extra undos in the given undo items.
USDUFE_PUBLIC
void removeSessionLeftOvers(
    const PXR_NS::UsdStageRefPtr& stage,
    const PXR_NS::SdfPath&        primPath,
    UsdUndoableItem*              undoableItem,
    bool                          extraEdits = true);

//! Return the USD prims predicate for the given UFE child filter.
//! Note: an empty filter or unknown filter will filter out everything.
//!       Yes, that means no-filter actually means filter everything out.
USDUFE_PUBLIC
PXR_NS::Usd_PrimFlagsPredicate getUsdPredicate(const Ufe::Hierarchy::ChildFilter& childFilter);

} // namespace USDUFE_NS_DEF

#endif // USDUFE_UFE_UTILS_H
