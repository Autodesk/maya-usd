//
// Copyright 2019 Autodesk
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
#include "Utils.h"

#include <memory>
#include <string>

#include <ufe/log.h>

#include <pxr/base/tf/stringUtils.h>
#include <pxr/usd/usdGeom/xformable.h>

#include <mayaUsdUtils/util.h>

PXR_NAMESPACE_USING_DIRECTIVE

MAYAUSD_NS_DEF {
namespace ufe {

//------------------------------------------------------------------------------
// Private helper functions
//------------------------------------------------------------------------------

void applyCommandRestriction(const UsdPrim& prim, const std::string& commandName)
{
    // early check to see if a particular node has any specs to contribute
    // to the final composed prim. e.g (a node in payload)
    if(!MayaUsdUtils::hasSpecs(prim)){
        std::string err = TfStringPrintf("Cannot %s [%s] because it doesn't have any specs to contribute to the composed prim.",
                                          commandName.c_str(),
                                          prim.GetName().GetString().c_str());
        throw std::runtime_error(err.c_str());
    }

    // if the current layer doesn't have any contributions
    if (!MayaUsdUtils::doesEditTargetLayerContribute(prim)) {
        auto strongestContributingLayer = MayaUsdUtils::strongestContributingLayer(prim);
        std::string err = TfStringPrintf("Cannot %s [%s] defined on another layer. " 
                                         "Please set [%s] as the target layer to proceed", 
                                         commandName.c_str(),
                                         prim.GetName().GetString().c_str(),
                                         strongestContributingLayer->GetDisplayName().c_str());
        throw std::runtime_error(err.c_str());
    }
    else
    {
        auto layers = MayaUsdUtils::layersWithContribution(prim);
        // if we have more than 2 layers that contributes to the final composed prim
        if (layers.size() > 1) {
            std::string layerDisplayNames;
            for (auto layer : layers) {
                layerDisplayNames.append("[" + layer->GetDisplayName() + "]" + ",");
            }
            layerDisplayNames.pop_back();
            std::string err = TfStringPrintf("Cannot %s [%s] with definitions or opinions on other layers. "
                                             "Opinions exist in %s",
                                             commandName.c_str(),
                                             prim.GetName().GetString().c_str(), 
                                             layerDisplayNames.c_str());
            throw std::runtime_error(err.c_str());
        }
    }
}

} // namespace ufe
} // namespace MayaUsd
