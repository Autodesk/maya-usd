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
#ifndef USDLAYEREDITOR_CUSTOMLAYERDATA_H
#define USDLAYEREDITOR_CUSTOMLAYERDATA_H

#include "LayerEditorAPI.h"

#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>

/// General utility functions used when using custom Layer data
namespace UsdLayerEditor {
namespace CustomLayerData {

/**
 * Get the String Array custom data on the layer
 *
 * @param layer     The layer the custom data is on
 * @param token     The key (dictionary) where the data is stored
 *
 * @return          Returns the string array (empty if not found)
 */
LayerEditorAPI PXR_NS::VtArray<std::string>
               getStringArray(const PXR_NS::SdfLayerRefPtr& layer, const PXR_NS::TfToken& token);

/**
 * Set the String Array custom data on the layer
 *
 * @param data      The array that we want to save in the custom data
 * @param layer     The layer the custom data will be stored in
 * @param token     The key (dictionary) where the data is stored
 */
LayerEditorAPI void setStringArray(
    const PXR_NS::VtArray<std::string>& data,
    const PXR_NS::SdfLayerRefPtr&       layer,
    const PXR_NS::TfToken&              token);

/**
 * Get the String custom data on the layer
 *
 * @param layer     The layer the custom data is on
 * @param token     The key (dictionary) where the data is stored
 *
 * @return          Returns the string (empty if not found)
 */
LayerEditorAPI std::string
               getString(const PXR_NS::SdfLayerRefPtr& layer, const PXR_NS::TfToken& token);

/**
 * Set the String Array custom data on the layer
 *
 * @param data      The string that we want to save in the custom data
 * @param layer     The layer the custom data will be stored in
 * @param token     The key (dictionary) where the data is stored
 */
LayerEditorAPI void setString(
    const std::string&            data,
    const PXR_NS::SdfLayerRefPtr& layer,
    const PXR_NS::TfToken&        token);

} // namespace CustomLayerData
} // namespace UsdLayerEditor

#endif // USDLAYEREDITOR_CUSTOMLAYERDATA_H
