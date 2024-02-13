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
#ifndef MAYAUSDAPI_UTILS_H
#define MAYAUSDAPI_UTILS_H

#include <mayaUsdAPI/api.h>

#include <pxr/usd/usd/attribute.h>

#include <ufe/attribute.h>
#include <ufe/rtid.h>

namespace MAYAUSDAPI_NS_DEF {

//! Returns the currently registered Ufe runtime id for Maya.
MAYAUSD_API_PUBLIC
Ufe::Rtid getMayaRunTimeId();

//! Returns whether or not the two src and dst Usd attributes are connected.
MAYAUSD_API_PUBLIC
bool isConnected(const PXR_NS::UsdAttribute& srcUsdAttr, const PXR_NS::UsdAttribute& dstUsdAttr);

//! Returns the Usd stage this attribute belongs to.
MAYAUSD_API_PUBLIC
PXR_NS::UsdStageWeakPtr usdStage(const Ufe::Attribute::Ptr& attribute);

//! Returns the native Usd attribute type of this attribute.
MAYAUSD_API_PUBLIC
PXR_NS::SdfValueTypeName usdAttributeType(const Ufe::Attribute::Ptr& attribute);

/*! Populates the passed value argument with the Usd value stored in the attribute at the given
 *  time, returning true if it succeeds.
 */
MAYAUSD_API_PUBLIC
bool getUsdValue(
    const Ufe::Attribute::Ptr& attribute,
    PXR_NS::VtValue&           value,
    PXR_NS::UsdTimeCode        time);

/*! Returns a Ufe command that can create a new material based on the given shader identifier or
 *  nullptr if the parent item is not a valid Usd item.
 *  The returned command is not executed; it is up to the caller to call execute().
 */
MAYAUSD_API_PUBLIC
Ufe::InsertChildCommand::Ptr addNewMaterialCommand(
    const Ufe::SceneItem::Ptr& parentItem,
    const std::string&         sdrShaderIdentifier);

/*! Returns a Ufe command that can create a material scope or nullptr if the parent item is not a
 *  valid Usd item.
 *  The returned command is not executed; it is up to the caller to call execute().
 */
MAYAUSD_API_PUBLIC
Ufe::SceneItemResultUndoableCommand::Ptr
createMaterialsScopeCommand(const Ufe::SceneItem::Ptr& parentItem);

/*! Returns a Ufe command that can create a new Usd stage with a new layer.
 *  The returned command is not executed; it is up to the caller to call execute().
 */
MAYAUSD_API_PUBLIC
Ufe::SceneItemResultUndoableCommand::Ptr
createStageWithNewLayerCommand(const Ufe::SceneItem::Ptr& parentItem);

//! Returns whether or not the given item is a materials scope.
MAYAUSD_API_PUBLIC
bool isMaterialsScope(const Ufe::SceneItem::Ptr& item);

//! Returns whether or not the given Ufe node type corresponds to a gateway Maya node.
MAYAUSD_API_PUBLIC
bool isAGatewayType(const std::string& mayaNodeType);

/*! Merges prims starting at a source path from a source layer and stage to a destination, returning
 *  true if it succeeds.
 */
MAYAUSD_API_PUBLIC
bool mergePrims(
    const PXR_NS::UsdStageRefPtr& srcStage,
    const PXR_NS::SdfLayerRefPtr& srcLayer,
    const PXR_NS::SdfPath&        srcPath,
    const PXR_NS::UsdStageRefPtr& dstStage,
    const PXR_NS::SdfLayerRefPtr& dstLayer,
    const PXR_NS::SdfPath&        dstPath);

//! Returns the directory part of the given file path.
MAYAUSD_API_PUBLIC
std::string getDir(const std::string& fullFilePath);

//! Takes in two absolute file paths and computes a relative path of the first one to second one.
MAYAUSD_API_PUBLIC
std::pair<std::string, bool>
makePathRelativeTo(const std::string& fileName, const std::string& relativeToDir);

/*! Returns the flag specifying whether Usd file paths should be saved as relative to the current
 *  edit target layer.
 */
MAYAUSD_API_PUBLIC
bool requireUsdPathsRelativeToEditTargetLayer();

} // namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSDAPI_UTILS_H
