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
#ifndef MAYAUSD_UTILS_UTILSERIALIZATION_H
#define MAYAUSD_UTILS_UTILSERIALIZATION_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>

/// General utility functions used when serializing Usd edits during a save operation
namespace UsdLayerEditor {
namespace Serialization {
struct StageLayersToSave;
} // namespace Serialization
} // namespace UsdLayerEditor

namespace MAYAUSD_NS_DEF {
namespace utils {

MAYAUSD_CORE_PUBLIC
std::string generateUniqueFileName(const std::string& basename);

/*! \brief Return if the relative-path plug is set to true on the proxy shape.
 */
MAYAUSD_CORE_PUBLIC
bool isProxyShapePathRelative(MayaUsdProxyShapeBase& proxyShape);

/*! \brief File path mode for the setNewProxyPath and isProxyPathModeRelative functions.
 *         kProxyPathRelative makes the file path relative to the Maya scene.
 *         kProxyPathAbsolute makes the file path absolute.
 *         kProxyPathFollowProxyShape reads the relative-path plug of the proxy shape
 *                                    to decide.
 *         kProxyPathFollowOptionVar reads the mayaUsd_MakePathRelativeToSceneFile
 *                                   options to decide.
 */
enum ProxyPathMode
{
    kProxyPathRelative,
    kProxyPathAbsolute,
    kProxyPathFollowProxyShape,
    kProxyPathFollowOptionVar
};

/*! \brief Convert the proxy path mode into a boolean telling if the path should be relative.
 *  \note the proxy node name is only required for the kProxyPathFollowProxyShape mode.
 */
MAYAUSD_CORE_PUBLIC
bool isProxyPathModeRelative(ProxyPathMode proxyPathMode, const MString& proxyNodeName);

/*! \brief Utility function to update the file path attribute on the proxy shape
    when an anonymous root layer gets exported to disk. Also optionally updates
    the target layer if the anonymous layer was the target layer.
 */
MAYAUSD_CORE_PUBLIC
void setNewProxyPath(
    const MString&        proxyNodeName,
    const MString&        newValue,
    ProxyPathMode         proxyPathMode,
    const SdfLayerRefPtr& layer,
    bool                  isTargetLayer);

/*! \brief Check the sublayer stack of the stage looking for any anonymous
    layers that will need to be saved.
 */
MAYAUSD_CORE_PUBLIC
void getLayersToSaveFromProxy(
    const std::string&                                proxyPath,
    UsdLayerEditor::Serialization::StageLayersToSave& layersInfo);

} // namespace utils
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UTILS_UTILSERIALIZATION_H
