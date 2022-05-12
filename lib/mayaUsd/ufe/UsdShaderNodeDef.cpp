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

#if (UFE_PREVIEW_VERSION_NUM >= 4010)
#include <mayaUsd/ufe/UsdShaderAttributeDef.h>
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

PXR_NAMESPACE_USING_DIRECTIVE

constexpr char UsdShaderNodeDef::kNodeDefCategoryShader[];

#if (UFE_PREVIEW_VERSION_NUM < 4010)
Ufe::Attribute::Type getUfeTypeForAttribute(const SdrShaderPropertyConstPtr& shaderProperty)
{
    const SdfValueTypeName typeName = shaderProperty->GetTypeAsSdfType().first;
    return usdTypeToUfe(typeName);
}
#endif

template <Ufe::AttributeDef::IOType IOTYPE>
Ufe::ConstAttributeDefs getAttrs(const SdrShaderNodeConstPtr& shaderNodeDef)
{
    Ufe::ConstAttributeDefs attrs;
    const bool              input = (IOTYPE == Ufe::AttributeDef::INPUT_ATTR);
    auto names = input ? shaderNodeDef->GetInputNames() : shaderNodeDef->GetOutputNames();
    attrs.reserve(names.size());
    for (const TfToken& name : names) {
        SdrShaderPropertyConstPtr property
            = input ? shaderNodeDef->GetShaderInput(name) : shaderNodeDef->GetShaderOutput(name);
        if (!property) {
            // Cannot do much if the pointer is null. This can happen if the type_info for a
            // class derived from SdrProperty is hidden inside a plugin library since
            // SdrNode::GetShaderInput has to downcast a NdrProperty pointer.
            continue;
        }
#if (UFE_PREVIEW_VERSION_NUM < 4010)
        std::ostringstream defaultValue;
        defaultValue << property->GetDefaultValue();
        Ufe::Attribute::Type type = getUfeTypeForAttribute(property);
        attrs.emplace_back(Ufe::AttributeDef::create(name, type, defaultValue.str(), IOTYPE));
#else
        attrs.emplace_back(Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property)));
#endif
    }
    return attrs;
}

UsdShaderNodeDef::UsdShaderNodeDef(const SdrShaderNodeConstPtr& shaderNodeDef)
    : Ufe::NodeDef()
#if (UFE_PREVIEW_VERSION_NUM < 4010)
    , fType(shaderNodeDef ? shaderNodeDef->GetName() : "")
#endif
    , fShaderNodeDef(shaderNodeDef)
#if (UFE_PREVIEW_VERSION_NUM < 4010)
    , fInputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::INPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
    , fOutputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
#endif
{
    if (!TF_VERIFY(fShaderNodeDef)) {
        throw std::runtime_error("Invalid shader node definition");
    }
}

UsdShaderNodeDef::~UsdShaderNodeDef() { }

#if (UFE_PREVIEW_VERSION_NUM < 4010)
const std::string& UsdShaderNodeDef::type() const { return fType; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::inputs() const { return fInputs; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::outputs() const { return fOutputs; }

#else

std::string UsdShaderNodeDef::type() const
{
    TF_AXIOM(fShaderNodeDef);
    return fShaderNodeDef->GetName();
}

std::size_t UsdShaderNodeDef::nbClassifications() const
{
    TF_AXIOM(fShaderNodeDef);

    // Based on a review of all items found in the Sdr registry as of USD 21.11:

    // UsdLux shaders provide 2 classification levels:
    //     - Context
    //     - SourceType
    if (fShaderNodeDef->GetFamily().IsEmpty()) {
        return 2;
    }

    // Regular shader nodes provide 3 classification levels:
    //    - family
    //    - role
    //    - sourceType
    return 3;
}

std::string UsdShaderNodeDef::classification(std::size_t level) const
{
    TF_AXIOM(fShaderNodeDef);
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
    return {};
}

std::vector<std::string> UsdShaderNodeDef::inputNames() const
{
    TF_AXIOM(fShaderNodeDef);
    std::vector<std::string> retVal;
    auto names = fShaderNodeDef->GetInputNames();
    retVal.reserve(names.size());
    for (auto&& n : names) {
        retVal.emplace_back(n.GetString());
    }
    return retVal;
}

bool UsdShaderNodeDef::hasInput(const std::string& name) const
{
    TF_AXIOM(fShaderNodeDef);
    return fShaderNodeDef->GetShaderInput(TfToken(name));
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::input(const std::string& name) const
{
    TF_AXIOM(fShaderNodeDef);
    if (SdrShaderPropertyConstPtr property = fShaderNodeDef->GetShaderInput(TfToken(name))) {
        return Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property));
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::inputs() const
{
    TF_AXIOM(fShaderNodeDef);
    return getAttrs<Ufe::AttributeDef::INPUT_ATTR>(fShaderNodeDef);
}

std::vector<std::string> UsdShaderNodeDef::outputNames() const
{
    TF_AXIOM(fShaderNodeDef);
    std::vector<std::string> retVal;
    auto names = fShaderNodeDef->GetOutputNames();
    retVal.reserve(names.size());
    for (auto&& n : names) {
        retVal.emplace_back(n.GetString());
    }
    return retVal;
}

bool UsdShaderNodeDef::hasOutput(const std::string& name) const
{
    TF_AXIOM(fShaderNodeDef);
    return fShaderNodeDef->GetShaderOutput(TfToken(name));
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::output(const std::string& name) const
{
    TF_AXIOM(fShaderNodeDef);
    if (SdrShaderPropertyConstPtr property = fShaderNodeDef->GetShaderOutput(TfToken(name))) {
        return Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property));
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::outputs() const
{
    TF_AXIOM(fShaderNodeDef);
    return getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(fShaderNodeDef);
}

Ufe::Value UsdShaderNodeDef::getMetadata(const std::string& key) const
{
    TF_AXIOM(fShaderNodeDef);
    const NdrTokenMap& metadata = fShaderNodeDef->GetMetadata();
    auto it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return Ufe::Value(it->second);
    }
    // TODO: Adapt UI metadata information found in SdrShaderNode to Ufe standards
    // TODO: Fix Mtlx parser in USD to populate UI metadata in SdrShaderNode
    return {};
}

bool UsdShaderNodeDef::hasMetadata(const std::string& key) const
{
    TF_AXIOM(fShaderNodeDef);
    const NdrTokenMap& metadata = fShaderNodeDef->GetMetadata();
    auto it = metadata.find(TfToken(key));
    return it != metadata.cend();
}

Ufe::SceneItem::Ptr UsdShaderNodeDef::createNode(
    const Ufe::SceneItem::Ptr& parent,
    const Ufe::PathComponent& name) const
{
    TF_AXIOM(fShaderNodeDef);
    UsdSceneItem::Ptr parentItem = std::dynamic_pointer_cast<UsdSceneItem>(parent);
    if (parentItem) {
        UsdUndoCreateFromNodeDefCommand::Ptr cmd
            = UsdUndoCreateFromNodeDefCommand::create(fShaderNodeDef, parentItem, name.string());
        if (cmd) {
            cmd->execute();
            return cmd->insertedChild();
        }
    }
    return {};
}

Ufe::InsertChildCommand::Ptr UsdShaderNodeDef::createNodeCmd(
    const Ufe::SceneItem::Ptr& parent,
    const Ufe::PathComponent& name) const
{
    TF_AXIOM(fShaderNodeDef);
    UsdSceneItem::Ptr parentItem = std::dynamic_pointer_cast<UsdSceneItem>(parent);
    if (parentItem) {
        return UsdUndoCreateFromNodeDefCommand::create(fShaderNodeDef, parentItem, name.string());
    }
    return {};
}
#endif

UsdShaderNodeDef::Ptr UsdShaderNodeDef::create(const SdrShaderNodeConstPtr& shaderNodeDef)
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
        SdrRegistry&        registry = SdrRegistry::GetInstance();
        SdrShaderNodePtrVec shaderNodeDefs = registry.GetShaderNodesByFamily();
        result.reserve(shaderNodeDefs.size());
        for (const SdrShaderNodeConstPtr& shaderNodeDef : shaderNodeDefs) {
            result.emplace_back(UsdShaderNodeDef::create(shaderNodeDef));
        }
    }
    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
