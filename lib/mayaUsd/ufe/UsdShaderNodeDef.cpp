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
#include <mayaUsd/ufe/UsdUndoCreateFromNodeDefCommand.h>
#include <mayaUsd/ufe/UsdUndoMaterialCommands.h>

#include <usdUfe/ufe/UsdShaderAttributeDef.h>
#endif

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>
#include <mayaUsd/utils/util.h>

#include <usdUfe/base/tokens.h>
#include <usdUfe/utils/Utils.h>

#include <pxr/base/tf/token.h>

#if PXR_VERSION >= 2505
#include <pxr/usd/sdr/declare.h>
#else
#include <pxr/usd/ndr/declare.h>
#endif

#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <ufe/attribute.h>
#include <ufe/runTimeMgr.h>

#include <map>

namespace MAYAUSD_NS_DEF {
namespace ufe {

MAYAUSD_VERIFY_CLASS_SETUP(Ufe::NodeDef, UsdShaderNodeDef);

PXR_NAMESPACE_USING_DIRECTIVE

constexpr char UsdShaderNodeDef::kNodeDefCategoryShader[];
constexpr char UsdShaderNodeDef::kNodeDefCategorySurface[];

template <Ufe::AttributeDef::IOType IOTYPE>
Ufe::ConstAttributeDefs getAttrs(const SdrShaderNodeConstPtr& shaderNodeDef)
{
    Ufe::ConstAttributeDefs attrs;
    const bool              input = (IOTYPE == Ufe::AttributeDef::INPUT_ATTR);
#if PXR_VERSION >= 2505
    auto names
        = input ? shaderNodeDef->GetShaderInputNames() : shaderNodeDef->GetShaderOutputNames();
    attrs.reserve(names.size());
    for (const TfToken& name : names) {
        SdrShaderPropertyConstPtr property
            = input ? shaderNodeDef->GetShaderInput(name) : shaderNodeDef->GetShaderOutput(name);
        TF_VERIFY(property);
#else
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
#endif
#ifndef UFE_V4_FEATURES_AVAILABLE
        std::ostringstream defaultValue;
        defaultValue << property->GetDefaultValue();
        Ufe::Attribute::Type type = UsdUfe::usdTypeToUfe(property);
        attrs.emplace_back(Ufe::AttributeDef::create(name, type, defaultValue.str(), IOTYPE));
#else
        attrs.emplace_back(
            Ufe::AttributeDef::ConstPtr(new UsdUfe::UsdShaderAttributeDef(property)));
#endif
    }
    return attrs;
}

UsdShaderNodeDef::UsdShaderNodeDef(const SdrShaderNodeConstPtr& shaderNodeDef)
    : Ufe::NodeDef()
    , _shaderNodeDef(shaderNodeDef)
#ifndef UFE_V4_FEATURES_AVAILABLE
    , _type(shaderNodeDef ? shaderNodeDef->GetName() : "")
    , _inputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::INPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
    , _outputs(
          shaderNodeDef ? getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(shaderNodeDef)
                        : Ufe::ConstAttributeDefs())
#endif
{
    if (!TF_VERIFY(_shaderNodeDef)) {
        throw std::runtime_error("Invalid shader node definition");
    }
}

#ifndef UFE_V4_FEATURES_AVAILABLE
const std::string& UsdShaderNodeDef::type() const { return _type; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::inputs() const { return _inputs; }

const Ufe::ConstAttributeDefs& UsdShaderNodeDef::outputs() const { return _outputs; }

#else

std::string UsdShaderNodeDef::type() const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    return _shaderNodeDef->GetIdentifier();
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
    TF_DEV_AXIOM(_shaderNodeDef);

    // Based on a review of all items found in the Sdr registry as of USD 21.11:

    // UsdLux shaders provide 2 classification levels:
    //     - Context
    //     - SourceType
    if (_shaderNodeDef->GetFamily().IsEmpty()) {
        return 2;
    }

    if (_isArnoldWithIssue1214(*_shaderNodeDef)) {
        // With 1214 active, we can provide 2 classification levels:
        //    - Name (as substitute for family)
        //    - SourceType
        return 2;
        // This might change in some future and fallback to the last case below.
    }

    // We have a client that stores Maya shader classifications in the Role. Let's split that into
    // subclassifications:
    auto splitRoles = UsdUfe::splitString(_shaderNodeDef->GetRole(), "/");

    // Regular shader nodes provide at least 3 classification levels:
    //    - family
    //    - role (which might be splittable at / boundaries)
    //    - sourceType
    return 2 + splitRoles.size();
}

std::string UsdShaderNodeDef::classification(std::size_t level) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    if (_shaderNodeDef->GetFamily().IsEmpty()) {
        switch (level) {
        // UsdLux:
        case 0: return _shaderNodeDef->GetContext().GetString();
        case 1: return _shaderNodeDef->GetSourceType().GetString();
        }
    }
    if (_isArnoldWithIssue1214(*_shaderNodeDef)) {
        switch (level) {
        // Arnold with issue 1214 active:
        case 0: return _shaderNodeDef->GetName();
        case 1: return _shaderNodeDef->GetSourceType().GetString();
        }
    }

    if (level == 0) {
        return _shaderNodeDef->GetFamily().GetString();
    }

    if (level == 1 && _shaderNodeDef->GetRole() == _shaderNodeDef->GetName()) {
        // See https://github.com/AcademySoftwareFoundation/MaterialX/issues/921
        return "other";
    }

    // We have a client that stores Maya shader classifications in the Role. Let's split that into
    // subclassifications:
    auto splitRoles = UsdUfe::splitString(_shaderNodeDef->GetRole(), "/");

    if (level - 1 < splitRoles.size()) {
        return UsdUfe::prettifyName(splitRoles[splitRoles.size() - level]);
    }

    if (level - 1 == splitRoles.size()) {
        // MAYA-126533: GLSLFX to go inside of USD
        if (_shaderNodeDef->GetSourceType() == _tokens->glslfx) {
            return "USD";
        }
        return UsdUfe::prettifyName(_shaderNodeDef->GetSourceType().GetString());
    }

    return {};
}

std::vector<std::string> UsdShaderNodeDef::inputNames() const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    std::vector<std::string> retVal;
#if PXR_VERSION >= 2505
    auto names = _shaderNodeDef->GetShaderInputNames();
#else
    auto               names = _shaderNodeDef->GetInputNames();
#endif
    retVal.reserve(names.size());
    for (auto&& n : names) {
        retVal.emplace_back(n.GetString());
    }
    return retVal;
}

bool UsdShaderNodeDef::hasInput(const std::string& name) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    return _shaderNodeDef->GetShaderInput(TfToken(name));
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::input(const std::string& name) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    if (SdrShaderPropertyConstPtr property = _shaderNodeDef->GetShaderInput(TfToken(name))) {
        return Ufe::AttributeDef::ConstPtr(new UsdUfe::UsdShaderAttributeDef(property));
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::inputs() const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    return getAttrs<Ufe::AttributeDef::INPUT_ATTR>(_shaderNodeDef);
}

std::vector<std::string> UsdShaderNodeDef::outputNames() const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    std::vector<std::string> retVal;
#if PXR_VERSION >= 2505
    auto names = _shaderNodeDef->GetShaderOutputNames();
#else
    auto               names = _shaderNodeDef->GetOutputNames();
#endif
    retVal.reserve(names.size());
    for (auto&& n : names) {
        retVal.emplace_back(n.GetString());
    }
    return retVal;
}

bool UsdShaderNodeDef::hasOutput(const std::string& name) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    return _shaderNodeDef->GetShaderOutput(TfToken(name));
}

Ufe::AttributeDef::ConstPtr UsdShaderNodeDef::output(const std::string& name) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    if (SdrShaderPropertyConstPtr property = _shaderNodeDef->GetShaderOutput(TfToken(name))) {
        return Ufe::AttributeDef::ConstPtr(new UsdUfe::UsdShaderAttributeDef(property));
    }
    return {};
}

Ufe::ConstAttributeDefs UsdShaderNodeDef::outputs() const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    return getAttrs<Ufe::AttributeDef::OUTPUT_ATTR>(_shaderNodeDef);
}

namespace {
typedef std::unordered_map<std::string, std::function<Ufe::Value(const PXR_NS::SdrShaderNode&)>>
    MetadataMap;
static const MetadataMap _metaMap = {
    // Conversion map between known USD metadata and its MaterialX equivalent:
    { UsdUfe::MetadataTokens->UIName.GetString(),
      [](const PXR_NS::SdrShaderNode& n) {
          std::string uiname;
          if (!n.GetLabel().IsEmpty() && n.GetLabel() != n.GetFamily()) {
              return n.GetLabel().GetString();
          }
          if (!n.GetFamily().IsEmpty() && !_isArnoldWithIssue1214(n)) {
              return UsdUfe::prettifyName(n.GetFamily().GetString());
          }
          return UsdUfe::prettifyName(n.GetName());
      } },
    { UsdUfe::MetadataTokens->UIDoc,
      [](const PXR_NS::SdrShaderNode& n) {
          return !n.GetHelp().empty() ? n.GetHelp() : Ufe::Value();
      } },
    // If Ufe decides to use another completely different convention, it can be added here:
};
} // namespace

Ufe::Value UsdShaderNodeDef::getMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
#if PXR_VERSION >= 2505
    const SdrTokenMap& metadata = _shaderNodeDef->GetMetadata();
#else
    const NdrTokenMap& metadata = _shaderNodeDef->GetMetadata();
#endif
    auto it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return Ufe::Value(it->second);
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end()) {
        return itMapper->second(*_shaderNodeDef);
    }

    return {};
}

bool UsdShaderNodeDef::hasMetadata(const std::string& key) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
#if PXR_VERSION >= 2505
    const SdrTokenMap& metadata = _shaderNodeDef->GetMetadata();
#else
    const NdrTokenMap& metadata = _shaderNodeDef->GetMetadata();
#endif
    auto it = metadata.find(TfToken(key));
    if (it != metadata.cend()) {
        return true;
    }

    MetadataMap::const_iterator itMapper = _metaMap.find(key);
    if (itMapper != _metaMap.end() && !itMapper->second(*_shaderNodeDef).empty()) {
        return true;
    }

    return false;
}

Ufe::SceneItem::Ptr UsdShaderNodeDef::createNode(
    const Ufe::SceneItem::Ptr& parent,
    const Ufe::PathComponent& name) const
{
    TF_DEV_AXIOM(_shaderNodeDef);
    auto parentItem = downcast(parent);
    if (parentItem) {
        UsdUndoCreateFromNodeDefCommand::Ptr cmd
            = UsdUndoCreateFromNodeDefCommand::create(_shaderNodeDef, parentItem, name.string());
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
    TF_DEV_AXIOM(_shaderNodeDef);
    auto parentItem = downcast(parent);
    if (parentItem) {
        if (UsdUndoAddNewMaterialCommand::CompatiblePrim(parentItem)) {
            return UsdUndoAddNewMaterialCommand::create(
                parentItem, _shaderNodeDef->GetIdentifier());
        }
        return UsdUndoCreateFromNodeDefCommand::create(
            _shaderNodeDef, parentItem, UsdUfe::sanitizeName(name.string()));
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
