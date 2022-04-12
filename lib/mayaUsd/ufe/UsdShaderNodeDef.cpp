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

#include "UsdShaderNodeDef.h"

#include "Global.h"
#include "Utils.h"

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <ufe/attribute.h>
#include <ufe/runTimeMgr.h>

#include <map>

namespace MAYAUSD_NS_DEF {
namespace ufe {

constexpr char UsdShaderNodeDef::kNodeDefCategoryShader[];

Ufe::Attribute::Type getUfeTypeForAttribute(const PXR_NS::SdrShaderPropertyConstPtr& shaderProperty)
{
    const PXR_NS::SdfValueTypeName typeName = shaderProperty->GetTypeAsSdfType().first;
    return usdTypeToUfe(typeName);
}

template <Ufe::AttributeDef::IOType IOTYPE>
Ufe::ConstAttributeDefs getAttrs(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef)
{
    Ufe::ConstAttributeDefs attrs;
    const bool              input = (IOTYPE == Ufe::AttributeDef::INPUT_ATTR);
    auto names = input ? shaderNodeDef->GetInputNames() : shaderNodeDef->GetOutputNames();
    for (const PXR_NS::TfToken& name : names) {
        PXR_NS::SdrShaderPropertyConstPtr property
            = input ? shaderNodeDef->GetShaderInput(name) : shaderNodeDef->GetShaderOutput(name);
        if (!property) {
            continue;
        }
        std::ostringstream defaultValue;
        defaultValue << property->GetDefaultValue();
        Ufe::Attribute::Type type = getUfeTypeForAttribute(property);
        attrs.push_back(Ufe::AttributeDef::create(name, type, defaultValue.str(), IOTYPE));
    }
    return attrs;
}

UsdShaderNodeDef::UsdShaderNodeDef(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef)
    : Ufe::NodeDef()
    , fType(shaderNodeDef ? shaderNodeDef->GetName() : "")
    , fShaderNodeDef(shaderNodeDef)
    , fInputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::INPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
    , fOutputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!TF_VERIFY(fShaderNodeDef)) {
        throw std::runtime_error("Invalid shader node definition");
    }
}

UsdShaderNodeDef::~UsdShaderNodeDef() { }

const std::string& UsdShaderNodeDef::type() const { return fType; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::inputs() const { return fInputs; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::outputs() const { return fOutputs; }

UsdShaderNodeDef::Ptr UsdShaderNodeDef::create(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef)
{
    try {
        return std::make_shared<UsdShaderNodeDef>(shaderNodeDef);
    } catch (const std::exception&) {
        return nullptr;
    }
}

Ufe::NodeDefs UsdShaderNodeDef::definitions(const std::string& category)
{
    Ufe::NodeDefs result;
    if (category == std::string(Ufe::NodeDefHandler::kNodeDefCategoryAll)
        || category == std::string(kNodeDefCategoryShader)) {
        PXR_NS::SdrRegistry&        registry = PXR_NS::SdrRegistry::GetInstance();
        PXR_NS::SdrShaderNodePtrVec shaderNodeDefs = registry.GetShaderNodesByFamily();
        for (const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef : shaderNodeDefs) {
            const std::string& type = shaderNodeDef->GetName();
            result.push_back(UsdShaderNodeDef::create(shaderNodeDef));
        }
    }
    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
