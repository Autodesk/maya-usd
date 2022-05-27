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
#include "UsdAttributes.h"

#include "UsdShaderNodeDefHandler.h"
#include "Utils.h"

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usdShade/utils.h>

#include <ufe/runTimeMgr.h>
#include <ufe/ufeAssert.h>

// Note: normally we would use this using directive, but here we cannot because
//		 one of our classes is called UsdAttribute which is exactly the same as
//		the one in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

#ifdef UFE_ENABLE_ASSERTS
static constexpr char kErrorMsgUnknown[] = "Unknown UFE attribute type encountered";
static constexpr char kErrorMsgInvalidAttribute[] = "Invalid USDAttribute!";
#endif

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdAttributes::UsdAttributes(const UsdSceneItem::Ptr& item)
    : Ufe::Attributes()
    , fItem(item)
#ifdef UFE_V4_FEATURES_AVAILABLE
    , fNodeDef(nullptr)
#endif
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!TF_VERIFY(item)) {
        throw std::runtime_error("Invalid attributes object");
    }
    fPrim = item->prim();
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::RunTimeMgr&         runTimeMgr = Ufe::RunTimeMgr::instance();
    Ufe::NodeDefHandler::Ptr nodeDefHandler = runTimeMgr.nodeDefHandler(item->runTimeId());
    fNodeDef = nodeDefHandler->definition(item);
#endif
}

UsdAttributes::~UsdAttributes() { }

/*static*/
UsdAttributes::Ptr UsdAttributes::create(const UsdSceneItem::Ptr& item)
{
    auto attrs = std::make_shared<UsdAttributes>(item);
    return attrs;
}

//------------------------------------------------------------------------------
// Ufe::Attributes overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdAttributes::sceneItem() const { return fItem; }

Ufe::Attribute::Type UsdAttributes::attributeType(const std::string& name)
{
    // If we've already created an attribute for this name, just return the type.
    auto iter = fAttributes.find(name);
    if (iter != std::end(fAttributes))
        return iter->second->type();

    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = fPrim.GetAttribute(tok);
    if (usdAttr.IsValid()) {
        return getUfeTypeForAttribute(usdAttr);
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    else {
        if (!fNodeDef) {
            return Ufe::Attribute::kInvalid;
        }
        auto baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(tok);
        Ufe::AttributeDef::ConstPtr port
            = baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input
            ? fNodeDef->input(baseNameAndType.first)
            : fNodeDef->output(baseNameAndType.first);
        if (port) {
            return port->type();
        }
    }
#endif
    return Ufe::Attribute::kInvalid;
}

Ufe::Attribute::Ptr UsdAttributes::attribute(const std::string& name)
{
    // early return if name is empty.
    if (name.empty()) {
        return nullptr;
    }

    // If we've already created an attribute for this name, just return it.
    auto iter = fAttributes.find(name);
    if (iter != std::end(fAttributes))
        return iter->second;

    // No attribute for the input name was found -> create one.
    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = fPrim.GetAttribute(tok);
    Ufe::Attribute::Type newAttrType;
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::AttributeDef::ConstPtr attributeDef = nullptr;
    if (fNodeDef) {
        Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
        for (auto const& input : inputs) {
            if (PXR_NS::UsdShadeUtils::GetFullName(
                    PXR_NS::TfToken(input->name()), PXR_NS::UsdShadeAttributeType::Input)
                == name) {
                attributeDef = input;
                newAttrType = input->type();
                break;
            }
        }
        if (!attributeDef) {
            Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
            for (auto const& output : outputs) {
                if (PXR_NS::UsdShadeUtils::GetFullName(
                        PXR_NS::TfToken(output->name()), PXR_NS::UsdShadeAttributeType::Output)
                    == name) {
                    attributeDef = output;
                    newAttrType = output->type();
                    break;
                }
            }
        }
    }
#endif
    if (usdAttr.IsValid()) {
        newAttrType = getUfeTypeForAttribute(usdAttr);
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    else if (!fNodeDef) {
        return nullptr;
    }
#endif

    Ufe::Attribute::Ptr newAttr = nullptr;
    if (newAttrType == Ufe::Attribute::kBool) {
        newAttr = UsdAttributeBool::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kInt) {
        newAttr = UsdAttributeInt::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kFloat) {
        newAttr = UsdAttributeFloat::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kDouble) {
        newAttr = UsdAttributeDouble::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kString) {
        newAttr = UsdAttributeString::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kColorFloat3) {
        newAttr = UsdAttributeColorFloat3::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kEnumString) {
        newAttr = UsdAttributeEnumString::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kInt3) {
        newAttr = UsdAttributeInt3::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kFloat3) {
        newAttr = UsdAttributeFloat3::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kDouble3) {
        newAttr = UsdAttributeDouble3::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else if (newAttrType == Ufe::Attribute::kGeneric) {
        newAttr = UsdAttributeGeneric::create(
            fItem,
            fPrim,
#ifdef UFE_V4_FEATURES_AVAILABLE
            attributeDef,
#endif
            usdAttr);
    } else {
        UFE_ASSERT_MSG(false, kErrorMsgUnknown);
    }
    fAttributes[name] = newAttr;
    return newAttr;
}

std::vector<std::string> UsdAttributes::attributeNames() const
{
    std::vector<std::string> names;
    std::set<std::string>    nameSet;
    std::string              name;
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (fNodeDef) {
        Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
        for (auto const& input : inputs) {
            name = PXR_NS::UsdShadeUtils::GetFullName(
                PXR_NS::TfToken(input->name()), PXR_NS::UsdShadeAttributeType::Input);
            names.push_back(name);
            nameSet.insert(name);
        }
        Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
        for (auto const& output : outputs) {
            name = PXR_NS::UsdShadeUtils::GetFullName(
                PXR_NS::TfToken(output->name()), PXR_NS::UsdShadeAttributeType::Output);
            names.push_back(name);
            nameSet.insert(name);
        }
    }
#endif
    if (fPrim) {
        auto primAttrs = fPrim.GetAttributes();
        for (const auto& attr : primAttrs) {
            name = attr.GetName();
            if (nameSet.find(name) == nameSet.end()) {
                names.push_back(name);
            }
        }
    }
    return names;
}

bool UsdAttributes::hasAttribute(const std::string& name) const
{
    PXR_NS::TfToken tkName(name);
    if (fPrim.HasAttribute(tkName)) {
        return true;
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    Ufe::ConstAttributeDefs inputs = fNodeDef->inputs();
    for (auto const& input : inputs) {
        if (PXR_NS::UsdShadeUtils::GetFullName(
                PXR_NS::TfToken(input->name()), PXR_NS::UsdShadeAttributeType::Input)
            == tkName.GetString()) {
            return true;
        }
    }
    Ufe::ConstAttributeDefs outputs = fNodeDef->outputs();
    for (auto const& output : outputs) {
        if (PXR_NS::UsdShadeUtils::GetFullName(
                PXR_NS::TfToken(output->name()), PXR_NS::UsdShadeAttributeType::Output)
            == tkName.GetString()) {
            return true;
        }
    }
#endif
    return false;
}

Ufe::Attribute::Type
UsdAttributes::getUfeTypeForAttribute(const PXR_NS::UsdAttribute& usdAttr) const
{
    if (usdAttr.IsValid()) {
        const PXR_NS::SdfValueTypeName typeName = usdAttr.GetTypeName();
        Ufe::Attribute::Type           type = usdTypeToUfe(typeName);
        // Special case for TfToken -> Enum. If it doesn't have any allowed
        // tokens, then use String instead.
        if (type == Ufe::Attribute::kEnumString) {
            auto attrDefn = fPrim.GetPrimDefinition().GetSchemaAttributeSpec(usdAttr.GetName());
            if (!attrDefn || !attrDefn->HasAllowedTokens())
                return Ufe::Attribute::kString;
        }
        return type;
    }

    UFE_ASSERT_MSG(false, kErrorMsgInvalidAttribute);
    return Ufe::Attribute::kInvalid;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
