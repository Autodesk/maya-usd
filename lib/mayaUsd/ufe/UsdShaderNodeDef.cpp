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

#if (UFE_PREVIEW_VERSION_NUM >= 4008)
#include <mayaUsd/ufe/UsdShaderAttributeDef.h>
#endif
#if (UFE_PREVIEW_VERSION_NUM >= 4009)
#include <mayaUsd/ufe/UsdUndoCreateFromNodeDefCommand.h>
#endif

#include "Global.h"
#include "Utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/usd/ndr/declare.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <ufe/attribute.h>
#include <ufe/runTimeMgr.h>

#include <map>

namespace MAYAUSD_NS_DEF {
namespace ufe {

constexpr char UsdShaderNodeDef::kNodeDefCategoryShader[];

#if (UFE_PREVIEW_VERSION_NUM < 4008)
Ufe::Attribute::Type getUfeTypeForAttribute(const PXR_NS::SdrShaderPropertyConstPtr& shaderProperty)
{
    const PXR_NS::SdfValueTypeName typeName = shaderProperty->GetTypeAsSdfType().first;
    return usdTypeToUfe(typeName);
}
#endif

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
            // Cannot do much if the pointer is null. This can happen if the type_info for a
            // class derived from SdrProperty is hidden inside a plugin library since
            // SdrNode::GetShaderInput has to downcast a NdrProperty pointer.
            continue;
        }
#if (UFE_PREVIEW_VERSION_NUM < 4008)
        std::ostringstream defaultValue;
        defaultValue << property->GetDefaultValue();
        Ufe::Attribute::Type type = getUfeTypeForAttribute(property);
        attrs.push_back(Ufe::AttributeDef::create(name, type, defaultValue.str(), IOTYPE));
#else
        attrs.push_back(Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property)));
#endif
    }
    return attrs;
}

UsdShaderNodeDef::UsdShaderNodeDef(const PXR_NS::SdrShaderNodeConstPtr& shaderNodeDef)
    : Ufe::NodeDef()
#if (UFE_PREVIEW_VERSION_NUM < 4008)
    , fType(shaderNodeDef ? shaderNodeDef->GetName() : "")
#endif
    , fShaderNodeDef(shaderNodeDef)
#if (UFE_PREVIEW_VERSION_NUM < 4008)
    , fInputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::INPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
    , fOutputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
#endif
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!TF_VERIFY(fShaderNodeDef)) {
        throw std::runtime_error("Invalid shader node definition");
    }
}

UsdShaderNodeDef::~UsdShaderNodeDef() { }

#if (UFE_PREVIEW_VERSION_NUM < 4008)
const std::string& UsdShaderNodeDef::type() const { return fType; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::inputs() const { return fInputs; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::outputs() const { return fOutputs; }

#else

std::string UsdShaderNodeDef::type() const
{
    // Returns the NodeDef name
    // - Universal context: UsdPreviewSurface, UsdUVTexture, UsdPrimvarReader_float2
    // - MaterialX context: ND_standard_surface_surfaceshader, ND_image_color3
    return fShaderNodeDef ? fShaderNodeDef->GetName() : std::string();
}

std::size_t UsdShaderNodeDef::nbClassifications() const
{
    // Current Sdr information available:
    //
    //      name        UsdPrimvarReader_float2   ND_image_color3   DomeLight
    //      family      UsdPrimvarReader          image             -
    //      context     usda                      pattern           light
    //      sourceType  glslfx                    mtlx              USD
    //      category    -                         -                 -
    //      identifier  UsdPrimvarReader_float2   ND_image_color3   PortalLight
    //      label       -                         image             -
    //      role        primvar                   texture           PortalLight

    // UsdLux has 2 classification levels:
    //     - Context
    //     - SourceType
    if (fShaderNodeDef->GetFamily().IsEmpty()) {
        return 2;
    }

    // For regular shader nodes, we seem to have 3 usable classifications:
    //    - family
    //    - role
    //    - sourceType
    // There are quite a few misclassified nodes in MaterialX, they end up
    // having their role set to the same value as the name.

    return 3;
}

std::string UsdShaderNodeDef::classification(std::size_t level) const
{
    if (fShaderNodeDef) {
        if (fShaderNodeDef->GetFamily().IsEmpty()) {
            switch (level) {
            // UsdLux:
            case 0: return fShaderNodeDef->GetContext().GetString();
            case 1: return fShaderNodeDef->GetSourceType().GetString();
            }
        }
        switch (level) {
        // UsdShade: These work with MaterialX and Preview surface. Need to recheck against
        // third-party renderers as we discover their shading nodes.
        case 0: {
            return fShaderNodeDef->GetFamily().GetString();
        }
        case 1: {
            if (fShaderNodeDef->GetRole() == fShaderNodeDef->GetName()) {
                // See https://github.com/AcademySoftwareFoundation/MaterialX/issues/921
                return "other";
            } else {
                return fShaderNodeDef->GetRole();
            }
        }
        case 2: {
            return fShaderNodeDef->GetSourceType().GetString();
        }
        }
    }
    return {};
}

std::vector<std::string> UsdShaderNodeDef::inputNames() const
{
    std::vector<std::string> retVal;
    if (fShaderNodeDef) {
        for (auto&& n : fShaderNodeDef->GetInputNames()) {
            retVal.push_back(n.GetString());
        }
    }
    return retVal;
}

bool UsdShaderNodeDef::hasInput(const std::string& name) const
{
    return fShaderNodeDef ? fShaderNodeDef->GetShaderInput(PXR_NS::TfToken(name)) : false;
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::input(const std::string& name) const
{
    if (fShaderNodeDef) {
        if (PXR_NS::SdrShaderPropertyConstPtr property
            = fShaderNodeDef->GetShaderInput(PXR_NS::TfToken(name))) {
            return Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property));
        }
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::inputs() const
{
    return fShaderNodeDef ? getAttrs<Ufe::AttributeDef::INPUT_ATTR>(fShaderNodeDef)
                          : Ufe::ConstAttributeDefs();
}

std::vector<std::string> UsdShaderNodeDef::outputNames() const
{
    std::vector<std::string> retVal;
    if (fShaderNodeDef) {
        for (auto&& n : fShaderNodeDef->GetOutputNames()) {
            retVal.push_back(n.GetString());
        }
    }
    return retVal;
}

bool UsdShaderNodeDef::hasOutput(const std::string& name) const
{
    return fShaderNodeDef ? fShaderNodeDef->GetShaderOutput(PXR_NS::TfToken(name)) : false;
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::output(const std::string& name) const
{
    if (fShaderNodeDef) {
        if (PXR_NS::SdrShaderPropertyConstPtr property
            = fShaderNodeDef->GetShaderOutput(PXR_NS::TfToken(name))) {
            return Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property));
        }
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::outputs() const
{
    return fShaderNodeDef ? getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(fShaderNodeDef)
                          : Ufe::ConstAttributeDefs();
}

Ufe::Value UsdShaderNodeDef::getMetadata(const std::string& key) const
{
    if (fShaderNodeDef) {
        const PXR_NS::NdrTokenMap& metadata = fShaderNodeDef->GetMetadata();
        auto it = metadata.find(PXR_NS::TfToken(key));
        if (it != metadata.cend()) {
            return Ufe::Value(it->second);
        }
    }
    // TODO: Adapt UI metadata information found in SdrShaderNode to Ufe standards
    // TODO: Fix Mtlx parser in USD to populate UI metadata in SdrShaderNode
    return {};
}

bool UsdShaderNodeDef::hasMetadata(const std::string& key) const
{
    if (fShaderNodeDef) {
        const PXR_NS::NdrTokenMap& metadata = fShaderNodeDef->GetMetadata();
        auto it = metadata.find(PXR_NS::TfToken(key));
        if (it != metadata.cend()) {
            return true;
        }
    }
    return false;
}
#endif

#if (UFE_PREVIEW_VERSION_NUM >= 4009)
Ufe::SceneItem::Ptr
UsdShaderNodeDef::createNode(const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name)
{
    Ufe::SceneItem::Ptr createdItem = nullptr;

    if (fShaderNodeDef) {
        UsdSceneItem::Ptr parentItem = std::dynamic_pointer_cast<UsdSceneItem>(parent);
        UsdUndoCreateFromNodeDefCommand::Ptr cmd
            = UsdUndoCreateFromNodeDefCommand::create(fShaderNodeDef, parentItem, name.string());
        if (cmd) {
            cmd->execute();
            createdItem = cmd->insertedChild();
        }
    }

    return createdItem;
}

Ufe::InsertChildCommand::Ptr
UsdShaderNodeDef::createNodeCmd(const Ufe::SceneItem::Ptr& parent, const Ufe::PathComponent& name)
{
    if (fShaderNodeDef) {
        UsdSceneItem::Ptr parentItem = std::dynamic_pointer_cast<UsdSceneItem>(parent);
        return UsdUndoCreateFromNodeDefCommand::create(fShaderNodeDef, parentItem, name.string());
    } else {
        return {};
    }
}
#endif

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
            result.push_back(UsdShaderNodeDef::create(shaderNodeDef));
        }
    }
    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
