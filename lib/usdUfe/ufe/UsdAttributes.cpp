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

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdAttributeHolder.h>
#include <usdUfe/ufe/Utils.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usdShade/utils.h>

#include <ufe/runTimeMgr.h>
#include <ufe/ufeAssert.h>

#ifdef UFE_V4_FEATURES_AVAILABLE
#include <usdUfe/ufe/UsdShaderAttributeDef.h>
#include <usdUfe/ufe/UsdShaderAttributeHolder.h>
#include <usdUfe/ufe/UsdUndoAttributesCommands.h>
#endif

// Note: normally we would use this using directive, but here we cannot because
//       one of our classes is called UsdAttribute which is exactly the same as
//      the one in USD.
// PXR_NAMESPACE_USING_DIRECTIVE

#ifdef UFE_ENABLE_ASSERTS
static constexpr char kErrorMsgUnknown[] = "Unknown UFE attribute type encountered";
#endif

namespace USDUFE_NS_DEF {

namespace {

#ifdef UFE_V4_FEATURES_AVAILABLE
std::pair<PXR_NS::SdrShaderPropertyConstPtr, PXR_NS::UsdShadeAttributeType>
_GetSdrPropertyAndType(const Ufe::SceneItem::Ptr& item, const std::string& tokName)
{
    auto shaderNode = usdShaderNodeFromSceneItem(item);
    if (shaderNode) {
        auto baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(PXR_NS::TfToken(tokName));
        switch (baseNameAndType.second) {
        case PXR_NS::UsdShadeAttributeType::Invalid: return { nullptr, baseNameAndType.second };
        case PXR_NS::UsdShadeAttributeType::Input:
            return { shaderNode->GetShaderInput(baseNameAndType.first), baseNameAndType.second };
        case PXR_NS::UsdShadeAttributeType::Output:
            return { shaderNode->GetShaderOutput(baseNameAndType.first), baseNameAndType.second };
        }
    }
    return { nullptr, PXR_NS::UsdShadeAttributeType::Invalid };
}
#endif
} // namespace

// Ensure that UsdAttributes is properly setup.
#ifdef UFE_V4_2_FEATURES_AVAILABLE
USDUFE_VERIFY_CLASS_SETUP(Ufe::Attributes_v4_2, UsdAttributes);
#else
USDUFE_VERIFY_CLASS_SETUP(Ufe::Attributes, UsdAttributes);
#endif

UsdAttributes::UsdAttributes(const UsdSceneItem::Ptr& item)
    : UFE_ATTRIBUTES_BASE()
    , _item(item)
{
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!TF_VERIFY(item)) {
        throw std::runtime_error("Invalid attributes object");
    }
    _prim = item->prim();
}

/*static*/
UsdAttributes::Ptr UsdAttributes::create(const UsdSceneItem::Ptr& item)
{
    auto attrs = std::make_shared<UsdAttributes>(item);
    return attrs;
}

//------------------------------------------------------------------------------
// Ufe::Attributes overrides
//------------------------------------------------------------------------------

Ufe::SceneItem::Ptr UsdAttributes::sceneItem() const { return _item; }

// Returns whether the given op is an inverse operation. i.e, it starts with "!invert!".
// Note: sizeof(invertPrefix) includes the final nul terminator, so we must subtract one.
static constexpr char   invertPrefix[] = "!invert!";
static constexpr size_t invertPrefixLen(sizeof(invertPrefix) / sizeof(invertPrefix[0]) - 1);
static bool             _IsInverseOp(const std::string& name)
{
    return PXR_NS::TfStringStartsWith(name, invertPrefix);
}

static PXR_NS::UsdAttribute _GetAttributeType(const PXR_NS::UsdPrim& prim, const std::string& name)
{
    PXR_NS::TfToken tok(name);
    bool            isInverseOp = _IsInverseOp(tok);
    // If it is an inverse operation, strip off the "!invert!" at the beginning
    // of name to get the associated attribute's name.
    return prim.GetAttribute(isInverseOp ? PXR_NS::TfToken(name.substr(invertPrefixLen)) : tok);
}

Ufe::Attribute::Type UsdAttributes::attributeType(const std::string& name)
{
    // If we've already created an attribute for this name, just return the type.
    auto iter = _usdAttributes.find(name);
    if (iter != std::end(_usdAttributes))
        return iter->second->type();

        // Shader definitions always win over created UsdAttributes:
#ifdef UFE_V4_FEATURES_AVAILABLE
    PXR_NS::SdrShaderPropertyConstPtr shaderProp = _GetSdrPropertyAndType(_item, name).first;
    if (shaderProp) {
        return UsdShaderAttributeDef(shaderProp).type();
    }
#endif

    // See if a UsdAttribute can be wrapped:
    PXR_NS::TfToken      tok(name);
    PXR_NS::UsdAttribute usdAttr = _GetAttributeType(_prim, name);
    if (usdAttr.IsValid()) {
        return usdTypeToUfe(usdAttr);
    }
    return Ufe::Attribute::kInvalid;
}

Ufe::Attribute::Ptr UsdAttributes::attribute(const std::string& name)
{
    // early return if name is empty.
    if (name.empty()) {
        return nullptr;
    }

    // If we've already created an attribute for this name, just return it.
    auto iter = _usdAttributes.find(name);
    if (iter != std::end(_usdAttributes))
        return iter->second;

        // Use a map of constructors to reduce the number of string comparisons. Since the naming
        // convention is extremely uniform, let's use a macro to simplify definition (and prevent
        // mismatch errors).
#define ADD_UFE_USD_CTOR(TYPE)                                                       \
    {                                                                                \
        Ufe::Attribute::k##TYPE,                                                     \
            [](const UsdSceneItem::Ptr& si, UsdAttributeHolder::UPtr&& attrHolder) { \
                return UsdAttribute##TYPE::create(si, std::move(attrHolder));        \
            }                                                                        \
    }

    static const std::unordered_map<
        std::string,
        std::function<Ufe::Attribute::Ptr(const UsdSceneItem::Ptr&, UsdAttributeHolder::UPtr&&)>>
        ctorMap = {
            ADD_UFE_USD_CTOR(Bool),
            ADD_UFE_USD_CTOR(Int),
#ifdef UFE_HAS_UNSIGNED_INT
            ADD_UFE_USD_CTOR(UInt),
#endif
            ADD_UFE_USD_CTOR(Float),
            ADD_UFE_USD_CTOR(Double),
            ADD_UFE_USD_CTOR(ColorFloat3),
            ADD_UFE_USD_CTOR(Int3),
            ADD_UFE_USD_CTOR(Float3),
            ADD_UFE_USD_CTOR(Double3),
            ADD_UFE_USD_CTOR(Generic),
#ifdef UFE_V4_FEATURES_AVAILABLE
            ADD_UFE_USD_CTOR(ColorFloat4),
            ADD_UFE_USD_CTOR(Filename),
            ADD_UFE_USD_CTOR(Float2),
            ADD_UFE_USD_CTOR(Float4),
            ADD_UFE_USD_CTOR(Matrix3d),
            ADD_UFE_USD_CTOR(Matrix4d),
#endif
            { Ufe::Attribute::kString,
              [](const UsdSceneItem::Ptr&   si,
                 UsdAttributeHolder::UPtr&& attrHolder) -> Ufe::Attribute::Ptr {
                  if (attrHolder->usdAttributeType() == PXR_NS::SdfValueTypeNames->String) {
                      return UsdAttributeString::create(si, std::move(attrHolder));
                  } else {
                      return UsdAttributeToken::create(si, std::move(attrHolder));
                  }
              } },
            { Ufe::Attribute::kEnumString,
              [](const UsdSceneItem::Ptr&   si,
                 UsdAttributeHolder::UPtr&& attrHolder) -> Ufe::Attribute::Ptr {
                  if (attrHolder->usdAttributeType() == PXR_NS::SdfValueTypeNames->String) {
                      return UsdAttributeEnumString::create(si, std::move(attrHolder));
                  } else {
                      return UsdAttributeEnumToken::create(si, std::move(attrHolder));
                  }
              } },
        };
#undef ADD_UFE_USD_CTOR

    Ufe::Attribute::Ptr newAttr;

#ifdef UFE_V4_FEATURES_AVAILABLE
    // The shader definition always wins over a created attribute:
    auto shaderPropAndType = _GetSdrPropertyAndType(_item, name);
    if (shaderPropAndType.first) {
        auto ctorIt = ctorMap.find(usdTypeToUfe(shaderPropAndType.first));
        UFE_ASSERT_MSG(ctorIt != ctorMap.end(), kErrorMsgUnknown);
        if (ctorIt != ctorMap.end()) {
            newAttr = ctorIt->second(
                _item,
                UsdShaderAttributeHolder::create(
                    _prim, shaderPropAndType.first, shaderPropAndType.second));
        }
    }
#endif

    if (!newAttr) {
        // No attribute for the input name was found -> create one.
        PXR_NS::TfToken tok(name);

        Ufe::Attribute::Type newAttrType;
        // check if this is a relationship instead of an attribute
        PXR_NS::UsdRelationship usdRel = _prim.GetRelationship(tok);
        if (usdRel.IsValid()) {
            newAttrType = Ufe::Attribute::kGeneric;
        } else {
            PXR_NS::UsdAttribute usdAttr = _GetAttributeType(_prim, name);
            if (!usdAttr.IsValid()) {
                return nullptr;
            }
            newAttrType = usdTypeToUfe(usdAttr);
        }

        auto ctorIt = ctorMap.find(newAttrType);
        UFE_ASSERT_MSG(ctorIt != ctorMap.end(), kErrorMsgUnknown);
        if (ctorIt != ctorMap.end())
            newAttr = ctorIt->second(_item, UsdAttributeHolder::create(_prim.GetProperty(tok)));
    }

#ifdef UFE_V4_FEATURES_AVAILABLE
    // If this is a Usd attribute (cannot change) then we cache it for future access.
    if (!canRemoveAttribute(_item, name)) {
        _usdAttributes[name] = newAttr;
    }
#else
    _usdAttributes[name] = newAttr;
#endif

    return newAttr;
}

std::vector<std::string> UsdAttributes::attributeNames() const
{
    std::vector<std::string> names;
    std::set<std::string>    nameSet;
    std::string              name;
#ifdef UFE_V4_FEATURES_AVAILABLE
    PXR_NS::SdrShaderNodeConstPtr shaderNode = usdShaderNodeFromSceneItem(_item);
    if (shaderNode) {
        auto addAttributeNames
            = [&names, &nameSet, &name](
                  auto const& shortNames, PXR_NS::UsdShadeAttributeType attrType) {
                  for (auto const& shortName : shortNames) {
                      name = PXR_NS::UsdShadeUtils::GetFullName(shortName, attrType);
                      names.push_back(name);
                      nameSet.insert(name);
                  }
              };
#if PXR_VERSION >= 2505
        addAttributeNames(shaderNode->GetShaderInputNames(), PXR_NS::UsdShadeAttributeType::Input);
        addAttributeNames(
            shaderNode->GetShaderOutputNames(), PXR_NS::UsdShadeAttributeType::Output);
#else
        addAttributeNames(shaderNode->GetInputNames(), PXR_NS::UsdShadeAttributeType::Input);
        addAttributeNames(shaderNode->GetOutputNames(), PXR_NS::UsdShadeAttributeType::Output);
#endif
    }
#endif
    if (_prim) {
        auto primAttrs = _prim.GetProperties();
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
    if (_prim.HasProperty(tkName)) {
        return true;
    }
#ifdef UFE_V4_FEATURES_AVAILABLE
    if (_GetSdrPropertyAndType(_item, name).first) {
        return true;
    }
#endif
    return false;
}

#ifdef UFE_V4_FEATURES_AVAILABLE
Ufe::AddAttributeUndoableCommand::Ptr
UsdAttributes::addAttributeCmd(const std::string& name, const Ufe::Attribute::Type& type)
{
    return UsdAddAttributeCommand::create(_item, name, type);
}
Ufe::UndoableCommand::Ptr UsdAttributes::removeAttributeCmd(const std::string& name)
{
    return UsdRemoveAttributeCommand::create(_item, name);
}
Ufe::RenameAttributeUndoableCommand::Ptr
UsdAttributes::renameAttributeCmd(const std::string& originalName, const std::string& newName)
{
    return UsdRenameAttributeCommand::create(_item, originalName, newName);
}
#endif

#ifdef UFE_V4_FEATURES_AVAILABLE
// Helpers for validation and execution:
bool UsdAttributes::canAddAttribute(const UsdSceneItem::Ptr& item, const Ufe::Attribute::Type& type)
{
    // See if we can add this attribute
    // We do not check for the attribute name uniqueness as in the case of the existence of another
    // attribute with the same name, a unique name (appending an incremental digit at the name end)
    // will automatically be provided.
    if (!item || !item->prim().IsActive()) {
        return false;
    }

    // Since we can always fallback to adding a custom attribute on any UsdPrim, we accept.
    return true;
}

std::string
UsdAttributes::getUniqueAttrName(const UsdSceneItem::Ptr& item, const std::string& attrName)
{
    // Then we need a create a unique one
    if (UsdAttributes(item).hasAttribute(attrName)) {
        const auto               kAttributeNames = UsdAttributes(item).attributeNames();
        PXR_NS::TfToken::HashSet attributeNames;

        for (const auto& attributeName : kAttributeNames) {
            attributeNames.insert(PXR_NS::TfToken(attributeName));
        }

        return UsdUfe::uniqueName(attributeNames, attrName);
    }
    return attrName;
}

Ufe::Attribute::Ptr UsdAttributes::doAddAttribute(
    const UsdSceneItem::Ptr&    item,
    const std::string&          name,
    const Ufe::Attribute::Type& type)
{
    // We have many ways of creating an attribute. Try to follow the rules whenever possible:
    // Ensure the name is unique.
    const std::string              kUniqueName = UsdAttributes::getUniqueAttrName(item, name);
    PXR_NS::TfToken                nameAsToken(kUniqueName);
    auto                           prim = item->prim();
    PXR_NS::UsdShadeNodeGraph      ngPrim(prim);
    PXR_NS::UsdShadeConnectableAPI connectApi(prim);
    if (ngPrim && connectApi) {
        auto baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(nameAsToken);
        if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
            PXR_NS::UsdShadeMaterial matPrim(prim);
            if (matPrim) {
                auto splitName = PXR_NS::TfStringSplit(kUniqueName, ":");
                if (splitName.size() == 3) {
                    if (splitName.back() == PXR_NS::UsdShadeTokens->surface) {
                        matPrim.CreateSurfaceOutput(PXR_NS::TfToken(splitName[1]));
                    } else if (splitName.back() == PXR_NS::UsdShadeTokens->displacement) {
                        matPrim.CreateDisplacementOutput(PXR_NS::TfToken(splitName[1]));
                    } else if (splitName.back() == PXR_NS::UsdShadeTokens->volume) {
                        matPrim.CreateVolumeOutput(PXR_NS::TfToken(splitName[1]));
                    }
                }
            }
            // Fallback to creating a nodegraph output.
            auto usdType = ufeTypeToUsd(type);
            auto output = connectApi.CreateOutput(
                baseNameAndType.first, usdType ? usdType : SdfValueTypeNames->Token);
            if (!usdType) {
                output.SetSdrMetadataByKey(
                    TfToken(UsdAttributeGeneric::nativeSdrTypeMetadata()), type);
            }
        } else if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
            auto usdType = ufeTypeToUsd(type);
            auto input = connectApi.CreateInput(
                baseNameAndType.first, usdType ? usdType : SdfValueTypeNames->Token);
            if (!usdType) {
                input.SetSdrMetadataByKey(
                    TfToken(UsdAttributeGeneric::nativeSdrTypeMetadata()), type);
            }
        }
    }

    // Fallback to creating a custom attribute.
    prim.CreateAttribute(nameAsToken, ufeTypeToUsd(type));

    return UsdAttributes(item).attribute(kUniqueName);
}
bool UsdAttributes::canRemoveAttribute(const UsdSceneItem::Ptr& item, const std::string& name)
{
    if (!item || !item->prim().IsActive() || !UsdAttributes(item).hasAttribute(name)) {
        return false;
    }

    PXR_NS::TfToken nameAsToken(name);
    auto            prim = item->prim();
    auto            attribute = prim.GetAttribute(nameAsToken);
    if (attribute.IsCustom()) {
        // Custom attributes can be removed.
        return true;
    }

    // We can also remove NodeGraph boundary attributes
    PXR_NS::UsdShadeNodeGraph      ngPrim(prim);
    PXR_NS::UsdShadeConnectableAPI connectApi(prim);
    if (ngPrim && connectApi) {
        auto baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(nameAsToken);
        if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
            PXR_NS::UsdShadeMaterial matPrim(prim);
            if (matPrim) {
                // Can not remove the 3 main material outputs as they are part of the schema
                if (baseNameAndType.first == PXR_NS::UsdShadeTokens->surface
                    || baseNameAndType.first == PXR_NS::UsdShadeTokens->displacement
                    || baseNameAndType.first == PXR_NS::UsdShadeTokens->volume) {
                    return false;
                }
            }
            for (auto&& authoredOutput : connectApi.GetOutputs(true)) {
                if (authoredOutput.GetFullName() == name) {
                    return true;
                }
            }
        } else if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
            for (auto&& authoredInput : connectApi.GetInputs(true)) {
                if (authoredInput.GetFullName() == name) {
                    return true;
                }
            }
        }
    }
    return false;
}
static void removeSrcAttrConnections(PXR_NS::UsdPrim& prim, const PXR_NS::UsdAttribute& srcUsdAttr)
{
    // Remove the connections with source srcUsdAttr.
    for (const auto& attribute : prim.GetAttributes()) {
        PXR_NS::UsdAttribute dstUsdAttr = attribute.As<PXR_NS::UsdAttribute>();

        if (isConnected(srcUsdAttr, dstUsdAttr)) {
            UsdShadeConnectableAPI::DisconnectSource(dstUsdAttr, srcUsdAttr);
            // Check if we can remove the property.
            if (canRemoveDstProperty(dstUsdAttr)) {
                // Remove the property.
                prim.RemoveProperty(dstUsdAttr.GetName());
            }
        }
    }
}

static void removeDstAttrConnections(const PXR_NS::UsdAttribute& dstUsdAttr)
{
    const auto                     kPrim = dstUsdAttr.GetPrim();
    PXR_NS::UsdShadeConnectableAPI connectApi(kPrim);

    if (!kPrim || !connectApi) {
        return;
    }

    const auto kPrimParent = kPrim.GetParent();

    if (!kPrimParent) {
        return;
    }

    UsdShadeSourceInfoVector sourcesInfo = connectApi.GetConnectedSources(dstUsdAttr);

    if (sourcesInfo.empty()) {
        return;
    }

    // The attribute is the connection destination.
    PXR_NS::UsdPrim connectedPrim = sourcesInfo[0].source.GetPrim();

    if (connectedPrim) {
        const std::string prefix = connectedPrim == kPrimParent
            ? PXR_NS::UsdShadeUtils::GetPrefixForAttributeType(PXR_NS::UsdShadeAttributeType::Input)
            : PXR_NS::UsdShadeUtils::GetPrefixForAttributeType(
                PXR_NS::UsdShadeAttributeType::Output);

        const std::string sourceName = prefix + sourcesInfo[0].sourceName.GetString();

        auto srcAttr = connectedPrim.GetAttribute(PXR_NS::TfToken(sourceName));

        if (srcAttr) {
            UsdShadeConnectableAPI::DisconnectSource(dstUsdAttr, srcAttr);
            // Check if we can remove the property.
            if (canRemoveSrcProperty(srcAttr)) {
                // Remove the property.
                connectedPrim.RemoveProperty(srcAttr.GetName());
            }
        }
    }
}

static void
removeAllChildrenConnections(const PXR_NS::UsdPrim& prim, const PXR_NS::UsdAttribute& srcUsdAttr)
{
    // Remove the connections with source srcUsdAttr for all the children.
    for (auto&& usdChild : prim.GetChildren()) {
        removeSrcAttrConnections(usdChild, srcUsdAttr);
    }
}

static void removeNodeGraphConnections(const PXR_NS::UsdAttribute& attr)
{
    const auto prim = attr.GetPrim();

    if (!prim) {
        return;
    }

    PXR_NS::UsdShadeNodeGraph ngPrim(prim);

    if (!ngPrim) {
        return;
    }

    auto primParent = prim.GetParent();

    if (!primParent) {
        return;
    }

    const auto baseNameAndType
        = PXR_NS::UsdShadeUtils::GetBaseNameAndType(PXR_NS::TfToken(attr.GetName()));

    if (baseNameAndType.second != PXR_NS::UsdShadeAttributeType::Output
        && baseNameAndType.second != PXR_NS::UsdShadeAttributeType::Input) {
        return;
    }

    // Remove the connections to the destination attribute.
    removeDstAttrConnections(attr);

    if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
        // Remove the connections from the source attribute.
        removeAllChildrenConnections(primParent, attr);
        removeSrcAttrConnections(primParent, attr);
    }

    if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
        removeAllChildrenConnections(prim, attr);
    }
}

void UsdAttributes::removeAttributesConnections(const PXR_NS::UsdPrim& prim)
{
    auto primParent = prim.GetParent();
    if (!primParent) {
        return;
    }

    const auto kPrimAttrs = prim.GetAttributes();

    // Remove all the connections to/from the attribute.
    for (const auto& attr : kPrimAttrs) {
        const auto kBaseNameAndType
            = PXR_NS::UsdShadeUtils::GetBaseNameAndType(PXR_NS::TfToken(attr.GetName()));

        if (kBaseNameAndType.second != PXR_NS::UsdShadeAttributeType::Output
            && kBaseNameAndType.second != PXR_NS::UsdShadeAttributeType::Input) {
            continue;
        }

        if (kBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
            // Remove the connections to the destination attribute.
            removeDstAttrConnections(attr);
        }

        if (kBaseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
            removeAllChildrenConnections(primParent, attr);
            // Remove the connections from the source attribute.
            removeSrcAttrConnections(primParent, attr);
        }
    }
}

bool UsdAttributes::doRemoveAttribute(const UsdSceneItem::Ptr& item, const std::string& name)
{
    PXR_NS::TfToken nameAsToken(name);
    auto            prim = item->prim();
    auto            attribute = prim.GetAttribute(nameAsToken);
    if (attribute.IsCustom()) {
        // Custom attributes can be removed.
        return prim.RemoveProperty(nameAsToken);
    }

    // We can also remove NodeGraph boundary attributes
    PXR_NS::UsdShadeNodeGraph      ngPrim(prim);
    PXR_NS::UsdShadeConnectableAPI connectApi(prim);
    if (ngPrim && connectApi) {
        auto baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(nameAsToken);
        if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
            auto output = connectApi.GetOutput(baseNameAndType.first);
            if (output) {
                removeNodeGraphConnections(attribute);
                connectApi.ClearSources(output);
                return prim.RemoveProperty(nameAsToken);
            }
        } else if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
            auto input = connectApi.GetInput(baseNameAndType.first);
            if (input) {
                removeNodeGraphConnections(attribute);
                connectApi.ClearSources(input);
                return prim.RemoveProperty(nameAsToken);
            }
        }
    }
    return false;
}
bool UsdAttributes::canRenameAttribute(
    const UsdSceneItem::Ptr& sceneItem,
    const std::string&       originalName,
    const std::string&       newName)
{
    // No need to rename the attribute.
    if (originalName == newName) {
        return false;
    }
    // Renaming meets the same conditions as attribute removal.
    return canRemoveAttribute(sceneItem, originalName);
}

static void setConnections(
    const PXR_NS::UsdPrim& prim,
    const SdfPath&         oldPropertyPath,
    const SdfPath&         newPropertyPath)
{
    for (const auto& attribute : prim.GetAttributes()) {
        PXR_NS::UsdAttribute  attr = attribute.As<PXR_NS::UsdAttribute>();
        PXR_NS::SdfPathVector sources;
        attr.GetConnections(&sources);

        bool hasChanged = false;
        // Check if the node attribute is connected to the original property path.
        for (size_t i = 0; i < sources.size(); ++i) {
            if (sources[i] == oldPropertyPath) {
                sources[i] = newPropertyPath;
                hasChanged = true;
            }
        }
        // Update the connections with the new property path.
        if (hasChanged) {
            attr.SetConnections(sources);
        }
    }
}

static void setConnectionsOfAllChildren(
    const PXR_NS::UsdPrim& prim,
    const SdfPath&         oldPropertyPath,
    const SdfPath&         newPropertyPath)
{
    // Update the connections with the new attribute name.
    for (const auto& node : prim.GetChildren()) {
        setConnections(node, oldPropertyPath, newPropertyPath);
    }
}

Ufe::Attribute::Ptr UsdAttributes::doRenameAttribute(
    const UsdSceneItem::Ptr& sceneItem,
    const std::string&       originalName,
    const std::string&       newName)
{
    // Avoid checks since we have already did them.
    PXR_NS::TfToken                nameAsToken(originalName);
    auto                           prim = sceneItem->prim();
    auto                           attribute = prim.GetAttribute(nameAsToken);
    PXR_NS::UsdShadeConnectableAPI connectApi(prim);
    UsdPrim                        primParent = prim.GetParent();

    // Ensure the newName is unique.
    const std::string uniqueNewName = UsdAttributes::getUniqueAttrName(sceneItem, newName);

    PXR_NS::UsdEditTarget editTarget = prim.GetStage()->GetEditTarget();
    const PXR_NS::TfToken kOldAttrName = attribute.GetName();
    const SdfPath         kPrimPath = attribute.GetPrim().GetPath();
    const SdfPath         kPropertyPath = kPrimPath.AppendProperty(attribute.GetName());
    auto                  propertyHandle = editTarget.GetPropertySpecForScenePath(kPropertyPath);
    auto                  baseNameAndType = PXR_NS::UsdShadeUtils::GetBaseNameAndType(nameAsToken);

    PXR_NS::UsdShadeNodeGraph ngPrim(prim);

    if (!propertyHandle) {
        return {};
    }

    UsdShadeSourceInfoVector sourcesInfo;

    // Save the connected sources since after the renaming we will lose them.
    if (connectApi) {
        sourcesInfo = connectApi.GetConnectedSources(attribute);
    }

    if (!propertyHandle->SetName(uniqueNewName)) {
        return {};
    }

    // Get the renamed attribute.
    auto renamedAttr = UsdAttributes(sceneItem).attribute(uniqueNewName);

    if (connectApi && ngPrim) {

        const PXR_NS::TfToken kNewNameAsToken = PXR_NS::TfToken(uniqueNewName);
        PXR_NS::UsdAttribute  usdRenamedAttribute = prim.GetAttribute(kNewNameAsToken);
        const SdfPath         kOldPropertyPath = kPrimPath.AppendProperty(kOldAttrName);
        const SdfPath         kNewPropertyPath = kPrimPath.AppendProperty(kNewNameAsToken);

        if (!sourcesInfo.empty()) {
            std::vector<UsdShadeConnectionSourceInfo> connectionsInfo;

            for (const auto& connectionInfo : sourcesInfo) {
                connectionsInfo.push_back(connectionInfo);
            }

            UsdShadeConnectableAPI::SetConnectedSources(usdRenamedAttribute, connectionsInfo);
        }

        // Given the unidirectional nature of connections, we discriminate whether the source is
        // input or output
        if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Input) {
            setConnectionsOfAllChildren(prim, kOldPropertyPath, kNewPropertyPath);
        }
        if (baseNameAndType.second == PXR_NS::UsdShadeAttributeType::Output) {
            setConnectionsOfAllChildren(prim.GetParent(), kOldPropertyPath, kNewPropertyPath);
            setConnections(prim.GetParent(), kOldPropertyPath, kNewPropertyPath);
        }
    }

    return renamedAttr;
}
#endif

#ifdef UFE_ATTRIBUTES_GET_ENUMS
UFE_ATTRIBUTES_BASE::Enums UsdAttributes::getEnums(const std::string& attrName) const
{
    auto shaderPropAndType = _GetSdrPropertyAndType(_item, attrName);
    if (shaderPropAndType.first) {
        const auto shaderPropertyHolder = UsdShaderAttributeHolder(
            _item->prim(), shaderPropAndType.first, shaderPropAndType.second);
        return shaderPropertyHolder.getEnums();
    } else {
        PXR_NS::UsdAttribute usdAttr = _GetAttributeType(_item->prim(), attrName);
        if (usdAttr.IsValid()) {
            const auto attrHolder = UsdAttributeHolder { usdAttr };
            return attrHolder.getEnums();
        }
    }
    return {};
}
#endif

} // namespace USDUFE_NS_DEF
