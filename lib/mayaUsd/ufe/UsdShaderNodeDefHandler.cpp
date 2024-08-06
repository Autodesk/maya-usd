//
// Copyright 2022 Autodesk
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

#include "UsdShaderNodeDefHandler.h"

#include "UsdShaderNodeDef.h"

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdr/registry.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::NodeDefHandler, UsdShaderNodeDefHandler);

UsdShaderNodeDefHandler::Ptr UsdShaderNodeDefHandler::create()
{
    return std::make_shared<UsdShaderNodeDefHandler>();
}

//------------------------------------------------------------------------------
// Ufe::ShaderNodeDefHandler overrides
//------------------------------------------------------------------------------

PXR_NS::SdrShaderNodeConstPtr
UsdShaderNodeDefHandler::usdDefinition(const Ufe::SceneItem::Ptr& item)
{
    return UsdUfe::usdShaderNodeFromSceneItem(item);
}

Ufe::NodeDef::Ptr UsdShaderNodeDefHandler::definition(const Ufe::SceneItem::Ptr& item) const
{
    PXR_NS::SdrShaderNodeConstPtr shaderNodeDef = usdDefinition(item);
    if (!shaderNodeDef) {
        return nullptr;
    }
    return UsdShaderNodeDef::create(shaderNodeDef);
}

Ufe::NodeDef::Ptr UsdShaderNodeDefHandler::definition(const std::string& type) const
{
    PXR_NS::SdrRegistry&          registry = PXR_NS::SdrRegistry::GetInstance();
    PXR_NS::TfToken               mxNodeType(type);
    PXR_NS::SdrShaderNodeConstPtr shaderNodeDef = registry.GetShaderNodeByIdentifier(mxNodeType);
    if (!shaderNodeDef) {
        return nullptr;
    }
    return UsdShaderNodeDef::create(shaderNodeDef);
}

Ufe::NodeDefs UsdShaderNodeDefHandler::definitions(const std::string& category) const
{
    return UsdShaderNodeDef::definitions(category);
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
