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
#include "customLayerData.h"

namespace UsdLayerEditor {
namespace CustomLayerData {

PXR_NS::VtArray<std::string>
getStringArray(const PXR_NS::SdfLayerRefPtr& layer, const PXR_NS::TfToken& token)
{
    if (!layer->HasCustomLayerData())
        return {};

    const PXR_NS::VtDictionary customData = layer->GetCustomLayerData();
    const auto                 it = customData.find(token.GetString());
    if (it == customData.end())
        return {};

    const PXR_NS::VtValue& array = it->second;
    if (!array.IsHolding<PXR_NS::VtArray<std::string>>())
        return {};

    return array.UncheckedGet<PXR_NS::VtArray<std::string>>();
}

} // namespace CustomLayerData
} // namespace UsdLayerEditor
