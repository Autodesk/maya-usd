//
// Copyright 2025 Autodesk
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

#ifndef MAYAUSD_UTILS_UTILCOMPONENTCREATOR_H
#define MAYAUSD_UTILS_UTILCOMPONENTCREATOR_H

#include "mayaUsd/mayaUsd.h"

#include <mayaUsd/base/api.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace MAYAUSD_NS_DEF {
namespace ComponentUtils {

/*! \brief Information about a template variant payload in a USD Component.
 */
struct TemplateVariantPayloadInfo
{
    std::string layerFilename; //!< Path to the payload layer file
    bool        isPayload;     //!< True if it's a payload, false if it's a reference
    std::string scopePath;     //!< The scope prim path

    TemplateVariantPayloadInfo()
        : isPayload(false)
    {
    }

    TemplateVariantPayloadInfo(
        const std::string& layer,
        bool               payload,
        const std::string& scope)
        : layerFilename(layer)
        , isPayload(payload)
        , scopePath(scope)
    {
    }
};

/*! \brief Returns whether the proxy shape at the given path identifies an Autodesk USD Component.
 */
MAYAUSD_CORE_PUBLIC
bool isAdskUsdComponent(const std::string& proxyPath);

/*! \brief Returns the ids of the USD layers that should be saved for the Autodesk USD Component.
 *
 *  \note Expects \p proxyPath to be a valid component path.
 */
MAYAUSD_CORE_PUBLIC
std::vector<std::string> getAdskUsdComponentLayersToSave(const std::string& proxyPath);

/*! \brief Saves the Autodesk USD Component identified by \p proxyPath.
 *
 *  \note Expects \p proxyPath to be a valid component path.
 */
MAYAUSD_CORE_PUBLIC
void saveAdskUsdComponent(const std::string& proxyPath);

/*! \brief Returns whether the stage is an new unsaved Autodesk USD Component.
 */
MAYAUSD_CORE_PUBLIC
bool isUnsavedAdskUsdComponent(const PXR_NS::UsdStageRefPtr stage);

/*! \brief Reloads the Autodesk USD Component identified by \p proxyPath.
 *
 *  \note Expects \p proxyPath to be a valid component path.
 */
MAYAUSD_CORE_PUBLIC
void reloadAdskUsdComponent(const std::string& proxyPath);

/*! \brief Previews the structure of the Autodesk USD Component identified by \p proxyPath,
 * when saved at the given location with the given name.
 * \return Returns the expected component hierarchy, formatted in json.
 */
MAYAUSD_CORE_PUBLIC
std::string previewSaveAdskUsdComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& proxyPath);

/*! \brief Moves the Autodesk USD Component to a new location with a new name.
 * \return Returns the new root layer file path on success, empty string on failure.
 */
MAYAUSD_CORE_PUBLIC
std::string moveAdskUsdComponent(
    const std::string& saveLocation,
    const std::string& componentName,
    const std::string& proxyPath);

/*! \brief Checks if the initial save dialog for components should be opened.
 * \return Returns true if the initial save dialog for components should be opened.
 */
MAYAUSD_CORE_PUBLIC
bool shouldDisplayComponentInitialSaveDialog(
    const PXR_NS::UsdStageRefPtr stage,
    const std::string&           proxyShapePath);

/*! \brief Returns the name of the materials scope used in the component at the given path
 */
MAYAUSD_CORE_PUBLIC
std::string getMaterialScopeName(const std::string& proxyPath);

/*! \brief Returns the name of the mesh scope used in the component at the given path.
 */
MAYAUSD_CORE_PUBLIC
std::string getMeshScopeName(const std::string& proxyPath);

/*! \brief Get all template variant payloads from a component stage.
 *
 * \param proxyPath The proxy shape path of the component stage
 * \return A map payload_layer_filename -> (layer_filename, is_payload, scope_path)
 */
MAYAUSD_CORE_PUBLIC
std::unordered_map<std::string, TemplateVariantPayloadInfo>
getTemplateVariantPayloadsFromComponentStage(const std::string& proxyPath);

} // namespace ComponentUtils
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UTILS_UTILCOMPONENTCREATOR_H
