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

#include <mayaUsd/ufe/Utils.h>
#ifdef UFE_V4_FEATURES_AVAILABLE
#include <mayaUsd/ufe/UsdShaderAttributeDef.h>
#include <mayaUsd/ufe/UsdUndoCreateFromNodeDefCommand.h>
#include <mayaUsd/ufe/UsdUndoMaterialCommands.h>
#endif

#include "Global.h"
#include "Utils.h"

#include <mayaUsd/base/tokens.h>
#include <mayaUsd/utils/util.h>

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
constexpr char UsdShaderNodeDef::kNodeDefCategorySurface[];

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
#ifndef UFE_V4_FEATURES_AVAILABLE
        std::ostringstream defaultValue;
        defaultValue << property->GetDefaultValue();
        Ufe::Attribute::Type type = usdTypeToUfe(property);
        attrs.emplace_back(Ufe::AttributeDef::create(name, type, defaultValue.str(), IOTYPE));
#else
        attrs.emplace_back(Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property)));
#endif
    }
    return attrs;
}

UsdShaderNodeDef::UsdShaderNodeDef(const SdrShaderNodeConstPtr& shaderNodeDef)
    : Ufe::NodeDef()
    , fShaderNodeDef(shaderNodeDef)
#ifndef UFE_V4_FEATURES_AVAILABLE
    , fType(shaderNodeDef ? shaderNodeDef->GetName() : "")
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

#ifndef UFE_V4_FEATURES_AVAILABLE
const std::string& UsdShaderNodeDef::type() const { return fType; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::inputs() const { return fInputs; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::outputs() const { return fOutputs; }

#else

std::string UsdShaderNodeDef::type() const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    return fShaderNodeDef->GetIdentifier();
}

namespace {
// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (arnold)
    (shader)
    (glslfx)
);
// clang-format on

// Arnold nodes seem to be incompletely registered, this affects the "classification" scheme used
// by the UFE abstraction. This has been identified as Arnold-USD issue 1214:
//    https://github.com/Autodesk/arnold-usd/issues/1214
// We will detect this in a way that should switch back to the normal classification scheme if the
// registration code is updated.
bool _isArnoldWithIssue1214(const PXR_NS::SdrShaderNode& shaderNodeDef)
{
    return shaderNodeDef.GetSourceType() == _tokens->arnold
        && shaderNodeDef.GetFamily() == _tokens->shader
        && shaderNodeDef.GetName() != _tokens->shader;
}
} // namespace

std::size_t UsdShaderNodeDef::nbClassifications() const
{
    TF_DEV_AXIOM(fShaderNodeDef);

    // Based on a review of all items found in the Sdr registry as of USD 21.11:

    // UsdLux shaders provide 2 classification levels:
    //     - Context
    //     - SourceType
    if (fShaderNodeDef->GetFamily().IsEmpty()) {
        return 2;
    }

    if (_isArnoldWithIssue1214(*fShaderNodeDef)) {
        // With 1214 active, we can provide 2 classification levels:
        //    - Name (as substitute for family)
        //    - SourceType
        return 2;
        // This might change in some future and fallback to the last case below.
    }

    // We have a client that stores Maya shader classifications in the Role. Let's split that into
    // subclassifications:
    auto splitRoles = splitString(fShaderNodeDef->GetRole(), "/");

    // Regular shader nodes provide at least 3 classification levels:
    //    - family
    //    - role (which might be splittable at / boundaries)
    //    - sourceType
    return 2 + splitRoles.size();
}

std::string UsdShaderNodeDef::classification(std::size_t level) const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    if (fShaderNodeDef->GetFamily().IsEmpty()) {
        switch (level) {
        // UsdLux:
        case 0: return fShaderNodeDef->GetContext().GetString();
        case 1: return fShaderNodeDef->GetSourceType().GetString();
        }
    }
    if (_isArnoldWithIssue1214(*fShaderNodeDef)) {
        switch (level) {
        // Arnold with issue 1214 active:
        case 0: return fShaderNodeDef->GetName();
        case 1: return fShaderNodeDef->GetSourceType().GetString();
        }
    }

    if (level == 0) {
        return fShaderNodeDef->GetFamily().GetString();
    }

    if (level == 1 && fShaderNodeDef->GetRole() == fShaderNodeDef->GetName()) {
        // See https://github.com/AcademySoftwareFoundation/MaterialX/issues/921
        return "other";
    }

    // We have a client that stores Maya shader classifications in the Role. Let's split that into
    // subclassifications:
    auto splitRoles = splitString(fShaderNodeDef->GetRole(), "/");

    if (level - 1 < splitRoles.size()) {
        return UsdMayaUtil::prettifyName(splitRoles[splitRoles.size() - level]);
    }

    if (level - 1 == splitRoles.size()) {
        // MAYA-126533: GLSLFX to go inside of USD
        if (fShaderNodeDef->GetSourceType() == _tokens->glslfx) {
            return "USD";
        }
        return fShaderNodeDef->GetSourceType().GetString();
    }

    return {};
}

std::vector<std::string> UsdShaderNodeDef::inputNames() const
{
    TF_DEV_AXIOM(fShaderNodeDef);
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
    TF_DEV_AXIOM(fShaderNodeDef);
    return fShaderNodeDef->GetShaderInput(TfToken(name));
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::input(const std::string& name) const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    if (SdrShaderPropertyConstPtr property = fShaderNodeDef->GetShaderInput(TfToken(name))) {
        return Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property));
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::inputs() const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    return getAttrs<Ufe::AttributeDef::INPUT_ATTR>(fShaderNodeDef);
}

std::vector<std::string> UsdShaderNodeDef::outputNames() const
{
    TF_DEV_AXIOM(fShaderNodeDef);
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
    TF_DEV_AXIOM(fShaderNodeDef);
    return fShaderNodeDef->GetShaderOutput(TfToken(name));
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::output(const std::string& name) const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    if (SdrShaderPropertyConstPtr property = fShaderNodeDef->GetShaderOutput(TfToken(name))) {
        return Ufe::AttributeDef::ConstPtr(new UsdShaderAttributeDef(property));
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::outputs() const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    return getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(fShaderNodeDef);
}

namespace {
typedef std::unordered_map<std::string, std::function<Ufe::Value(const PXR_NS::SdrShaderNode&)>>
    MetadataMap;
static const MetadataMap _metaMap = {
    // Conversion map between known USD metadata and its MaterialX equivalent:
    { MayaUsdMetadata->UIName.GetString(),
      [](const PXR_NS::SdrShaderNode& n) {
          std::string uiname;
          if (!n.GetLabel().IsEmpty()) {
              return n.GetLabel().GetString();
          }
          if (!n.GetFamily().IsEmpty() && !_isArnoldWithIssue1214(n)) {
              return UsdMayaUtil::prettifyName(n.GetFamily().GetString());
          }
          return UsdMayaUtil::prettifyName(n.GetName());
      } },
    { "doc",
      [](const PXR_NS::SdrShaderNode& n) {
          return !n.GetHelp().empty() ? n.GetHelp() : Ufe::Value();
      } },
    // If Ufe decides to use another completely different convention, it can be added here:
};
} // namespace

Ufe::Value UsdShaderNodeDef::getMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    const NdrTokenMap& metadata = fShaderNodeDef->GetMetadata();
    auto it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return Ufe::Value(it->second);
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end()) {
        return itMapper->second(*fShaderNodeDef);
    }

    return {};
}

bool UsdShaderNodeDef::hasMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(fShaderNodeDef);
    const NdrTokenMap& metadata = fShaderNodeDef->GetMetadata();
    auto it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return true;
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end() && !itMapper->second(*fShaderNodeDef).empty()) {
        return true;
    }

    return false;
}

Ufe::SceneItem::Ptr UsdShaderNodeDef::createNode(
    const Ufe::SceneItem::Ptr& parent,
    const Ufe::PathComponent& name) const
{
    TF_DEV_AXIOM(fShaderNodeDef);
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
    TF_DEV_AXIOM(fShaderNodeDef);
    UsdSceneItem::Ptr parentItem = std::dynamic_pointer_cast<UsdSceneItem>(parent);
    if (parentItem) {
        if (UsdUndoAddNewMaterialCommand::CompatiblePrim(parentItem)) {
            return UsdUndoAddNewMaterialCommand::create(
                parentItem, fShaderNodeDef->GetIdentifier());
        }
        return UsdUndoCreateFromNodeDefCommand::create(
            fShaderNodeDef, parentItem, UsdMayaUtil::SanitizeName(name.string()));
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
    static std::unordered_set<std::string> validCategories = {
        Ufe::NodeDefHandler::kNodeDefCategoryAll, kNodeDefCategoryShader, kNodeDefCategorySurface
    };

    Ufe::NodeDefs result;
    if (validCategories.count(category)) {
        SdrShaderNodePtrVec shaderNodeDefs;

        if (category == kNodeDefCategorySurface) {
            shaderNodeDefs = UsdMayaUtil::GetSurfaceShaderNodeDefs();
        } else {
            SdrRegistry& registry = SdrRegistry::GetInstance();
            shaderNodeDefs = registry.GetShaderNodesByFamily();
        }

        result.reserve(shaderNodeDefs.size());
        for (const SdrShaderNodeConstPtr& shaderNodeDef : shaderNodeDefs) {
            result.emplace_back(UsdShaderNodeDef::create(shaderNodeDef));
        }
    }
    return result;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
