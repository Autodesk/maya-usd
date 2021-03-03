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
#include <pxr/usd/sdf/fileFormat.h>
#include <pxr/usd/sdf/layer.h>

/// General utility functions used when serializing Usd edits during a save operation
namespace MAYAUSD_NS_DEF {
namespace utils {

/*! \brief Helps suggest a folder to export anonymous layers to.  Checks in order:
    1. File-backed root layer folder.
    2. Current Maya scene folder.
    3. Current Maya workspace scenes folder.
 */
MAYAUSD_CORE_PUBLIC
std::string suggestedStartFolder(PXR_NS::UsdStageRefPtr stage);

/*! \brief Queries Maya for the current workspace "scenes" folder.
 */
MAYAUSD_CORE_PUBLIC
std::string getSceneFolder();

MAYAUSD_CORE_PUBLIC
std::string generateUniqueFileName(const std::string& basename);

/*! \brief Queries the Maya optionVar that decides what the internal format
    of a .usd file should be, either "usdc" or "usda".
 */
MAYAUSD_CORE_PUBLIC
std::string usdFormatArgOption();

enum USDUnsavedEditsOption
{
    kSaveToUSDFiles = 1,
    kSaveToMayaSceneFile,
    kIgnoreUSDEdits
};
/*! \brief Queries the Maya optionVar that decides which saving option Maya
    shoudl use for Usd edits.
 */
MAYAUSD_CORE_PUBLIC
USDUnsavedEditsOption serializeUsdEditsLocationOption();

/*! \brief Save an anonymous layer to disk and update the sublayer path array
    in the parent layer.
 */
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerRefPtr saveAnonymousLayer(
    PXR_NS::SdfLayerRefPtr anonLayer,
    PXR_NS::SdfLayerRefPtr parentLayer,
    const std::string&     basename,
    std::string            formatArg = "");

/*! \brief Save an anonymous layer to disk and update the sublayer path array
    in the parent layer.
 */
MAYAUSD_CORE_PUBLIC
PXR_NS::SdfLayerRefPtr saveAnonymousLayer(
    PXR_NS::SdfLayerRefPtr anonLayer,
    const std::string&     path,
    PXR_NS::SdfLayerRefPtr parentLayer,
    std::string            formatArg = "");

struct stageLayersToSave
{
    std::vector<std::pair<PXR_NS::SdfLayerRefPtr, PXR_NS::SdfLayerRefPtr>> anonLayers;
    std::vector<PXR_NS::SdfLayerRefPtr>                                    dirtyFileBackedLayers;
};

/*! \brief Check the sublayer stack of the stage looking for any anonymnous
    layers that will need to be saved.
 */
MAYAUSD_CORE_PUBLIC
void getLayersToSaveFromProxy(PXR_NS::UsdStageRefPtr stage, stageLayersToSave& layersInfo);

} // namespace utils
} // namespace MAYAUSD_NS_DEF

#endif // MAYAUSD_UTILS_UTILSERIALIZATION_H
