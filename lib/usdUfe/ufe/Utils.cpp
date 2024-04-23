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
#include "Utils.h"

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdAttribute.h>
#include <usdUfe/ufe/UsdAttributes.h>
#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/utils/editability.h>
#include <usdUfe/utils/layers.h>
#include <usdUfe/utils/loadRules.h>
#include <usdUfe/utils/usdUtils.h>

#include <pxr/base/tf/token.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/pcp/site.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primCompositionQuery.h>
#include <pxr/usd/usd/resolver.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdShade/shader.h>

#include <ufe/attributeInfo.h>
#include <ufe/pathSegment.h>
#include <ufe/pathString.h>
#include <ufe/selection.h>

#include <cctype>
#include <regex>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

constexpr auto kIllegalUFEPath = "Illegal UFE run-time path %s.";
#ifdef UFE_SCENEITEM_HAS_METADATA
constexpr auto kErrorMsgInvalidValueType = "Unexpected Ufe::Value type";
#endif

typedef std::unordered_map<TfToken, SdfValueTypeName, TfToken::HashFunctor> TokenToSdfTypeMap;

bool stringBeginsWithDigit(const std::string& inputString)
{
    if (inputString.empty()) {
        return false;
    }

    const char& firstChar = inputString.front();
    if (std::isdigit(static_cast<unsigned char>(firstChar))) {
        return true;
    }

    return false;
}

// This function calculates the position index for a given layer across all
// the site's local LayerStacks
uint32_t findLayerIndex(const UsdPrim& prim, const SdfLayerHandle& layer)
{
    uint32_t position { 0 };

    const PcpPrimIndex& primIndex = prim.ComputeExpandedPrimIndex();

    // iterate through the expanded primIndex
    for (PcpNodeRef node : primIndex.GetNodeRange()) {

        TF_AXIOM(node);

        const PcpLayerStackSite&   site = node.GetSite();
        const PcpLayerStackRefPtr& layerStack = site.layerStack;

        // iterate through the "local" Layer stack for each site
        // to find the layer
        for (SdfLayerRefPtr const& l : layerStack->GetLayers()) {
            if (l == layer) {
                return position;
            }
            ++position;
        }
    }

    return position;
}

int gWaitCursorCount = 0;

UsdUfe::StageAccessorFn      gStageAccessorFn = nullptr;
UsdUfe::StagePathAccessorFn  gStagePathAccessorFn = nullptr;
UsdUfe::UfePathToPrimFn      gUfePathToPrimFn = nullptr;
UsdUfe::TimeAccessorFn       gTimeAccessorFn = nullptr;
UsdUfe::IsAttributeLockedFn  gIsAttributeLockedFn = nullptr;
UsdUfe::SaveStageLoadRulesFn gSaveStageLoadRulesFn = nullptr;
UsdUfe::IsRootChildFn        gIsRootChildFn = nullptr;
UsdUfe::UniqueChildNameFn    gUniqueChildNameFn = nullptr;
UsdUfe::WaitCursorFn         gStartWaitCursorFn = nullptr;
UsdUfe::WaitCursorFn         gStopWaitCursorFn = nullptr;

UsdUfe::DisplayMessageFn gDisplayMessageFn[static_cast<int>(UsdUfe::MessageType::nbTypes)]
    = { nullptr };

} // anonymous namespace

namespace USDUFE_NS_DEF {

//------------------------------------------------------------------------------
// Utility Functions
//------------------------------------------------------------------------------

void setStageAccessorFn(StageAccessorFn fn)
{
    if (nullptr == fn) {
        throw std::invalid_argument("Path to prim function cannot be empty.");
    }
    gStageAccessorFn = fn;
};

PXR_NS::UsdStageWeakPtr getStage(const Ufe::Path& path)
{
#if !defined(NDEBUG)
    assert(gStageAccessorFn != nullptr);
#endif
    return gStageAccessorFn(path);
}

void setStagePathAccessorFn(StagePathAccessorFn fn)
{
    if (nullptr == fn) {
        throw std::invalid_argument("Path to prim function cannot be empty.");
    }
    gStagePathAccessorFn = fn;
}

Ufe::Path stagePath(PXR_NS::UsdStageWeakPtr stage)
{
#if !defined(NDEBUG)
    assert(gStagePathAccessorFn != nullptr);
#endif
    return gStagePathAccessorFn(stage);
}

Ufe::PathSegment usdPathToUfePathSegment(const SdfPath& usdPath, int instanceIndex)
{
    const Ufe::Rtid   usdRuntimeId = getUsdRunTimeId();
    static const char separator = SdfPathTokens->childDelimiter.GetText()[0u];

    if (usdPath.IsEmpty()) {
        // Return an empty segment.
        return Ufe::PathSegment(Ufe::PathSegment::Components(), usdRuntimeId, separator);
    }

    std::string pathString = usdPath.GetString();

    if (instanceIndex >= 0) {
        // Note here that we're taking advantage of the fact that identifiers
        // in SdfPaths must be C/Python identifiers; that is, they must *not*
        // begin with a digit. This means that when we see a path component at
        // the end of a USD path segment that does begin with a digit, we can
        // be sure that it represents an instance index and not a prim or other
        // USD entity.
        pathString += TfStringPrintf("%c%d", separator, instanceIndex);
    }

    return Ufe::PathSegment(pathString, usdRuntimeId, separator);
}

Ufe::Path stripInstanceIndexFromUfePath(const Ufe::Path& path)
{
    if (path.empty()) {
        return path;
    }

    // As with usdPathToUfePathSegment() above, we're taking advantage of the
    // fact that identifiers in SdfPaths must be C/Python identifiers; that is,
    // they must *not* begin with a digit. This means that when we see a path
    // component at the end of a USD path segment that does begin with a digit,
    // we can be sure that it represents an instance index and not a prim or
    // other USD entity.
    if (stringBeginsWithDigit(path.back().string())) {
        return path.pop();
    }

    return path;
}

void setUfePathToPrimFn(UfePathToPrimFn fn)
{
    if (nullptr == fn) {
        throw std::invalid_argument("Path to prim function cannot be empty.");
    }
    gUfePathToPrimFn = fn;
}

UsdPrim ufePathToPrim(const Ufe::Path& path)
{
#if !defined(NDEBUG)
    assert(gUfePathToPrimFn != nullptr);
#endif
    return gUfePathToPrimFn(path);
}

void setTimeAccessorFn(TimeAccessorFn fn)
{
    if (nullptr == fn) {
        throw std::invalid_argument("Time accessor function cannot be empty.");
    }
    gTimeAccessorFn = fn;
}

PXR_NS::UsdTimeCode getTime(const Ufe::Path& path)
{
#if !defined(NDEBUG)
    assert(gTimeAccessorFn != nullptr);
#endif
    return gTimeAccessorFn(path);
}

void setIsAttributeLockedFn(IsAttributeLockedFn fn)
{
    // This function is allowed to be null in which case return default (false).
    gIsAttributeLockedFn = fn;
}

bool isAttributedLocked(const PXR_NS::UsdAttribute& attr, std::string* errMsg /*= nullptr*/)
{
    // If we have (optional) attribute is locked function, use it.
    // Otherwise use the default one supplied by UsdUfe.
    if (gIsAttributeLockedFn)
        return gIsAttributeLockedFn(attr, errMsg);
    return Editability::isAttributeLocked(attr, errMsg);
}

void setSaveStageLoadRulesFn(SaveStageLoadRulesFn fn)
{
    // This function is allowed to be null in which case nothing extra is done
    // to save the load rules when loading/unloading a payload.
    gSaveStageLoadRulesFn = fn;
}

void saveStageLoadRules(const PXR_NS::UsdStageRefPtr& stage)
{
    if (gSaveStageLoadRulesFn)
        gSaveStageLoadRulesFn(stage);
}

void setIsRootChildFn(IsRootChildFn fn)
{
    // This function is allowed to be null in which case, the default implementation
    // is used (isRootChildDefault()).
    gIsRootChildFn = fn;
}

bool isRootChild(const Ufe::Path& path)
{
    return gIsRootChildFn ? gIsRootChildFn(path) : isRootChildDefault(path);
}

bool isRootChildDefault(const Ufe::Path& path)
{
    // When called we make the assumption that we are given a valid
    // path and we are only testing whether or not we are a root child.
    auto segments = path.getSegments();
    if (segments.size() != 2) {
        TF_RUNTIME_ERROR(kIllegalUFEPath, path.string().c_str());
    }
    return (segments[1].size() == 1);
}

int ufePathToInstanceIndex(const Ufe::Path& path, UsdPrim* prim)
{
    int instanceIndex = UsdImagingDelegate::ALL_INSTANCES;

    const UsdPrim usdPrim = ufePathToPrim(path);
    if (prim) {
        *prim = usdPrim;
    }
    if (!usdPrim || !usdPrim.IsA<UsdGeomPointInstancer>()) {
        return instanceIndex;
    }

    // Once more as above in usdPathToUfePathSegment() and
    // stripInstanceIndexFromUfePath(), a path component at the tail of the
    // path that begins with a digit is assumed to represent an instance index.
    const std::string& tailComponentString = path.back().string();
    if (stringBeginsWithDigit(path.back().string())) {
        instanceIndex = std::stoi(tailComponentString);
    }

    return instanceIndex;
}

bool splitNumericalSuffix(const std::string srcName, std::string& base, std::string& suffix)
{
    // Compiled regular expression to find a numerical suffix to a path component.
    // It searches for any number of characters followed by a single non-numeric,
    // then one or more digits at end of string.
    std::regex re("(.*)([^0-9])([0-9]+)$");
    base = srcName;
    std::smatch match;
    if (std::regex_match(srcName, match, re)) {
        base = match[1].str() + match[2].str();
        suffix = match[3].str();
        return true;
    }
    return false;
}

std::string uniqueName(const TfToken::HashSet& existingNames, std::string srcName)
{
    std::string base, suffixStr;
    int         suffix { 1 };
    size_t      lenSuffix { 1 };
    if (splitNumericalSuffix(srcName, base, suffixStr)) {
        lenSuffix = suffixStr.length();
        suffix = std::stoi(suffixStr) + 1;
    }

    // Create a suffix string from the number keeping the same number of digits as
    // numerical suffix from input srcName (padding with 0's if needed).
    suffixStr = std::to_string(suffix);
    suffixStr = std::string(lenSuffix - std::min(lenSuffix, suffixStr.length()), '0') + suffixStr;
    std::string dstName = base + suffixStr;
    while (existingNames.count(TfToken(dstName)) > 0) {
        suffixStr = std::to_string(++suffix);
        suffixStr
            = std::string(lenSuffix - std::min(lenSuffix, suffixStr.length()), '0') + suffixStr;
        dstName = base + suffixStr;
    }
    return dstName;
}

void setUniqueChildNameFn(UniqueChildNameFn fn)
{
    // This function is allowed to be null in which case, the default implementation
    // is used (uniqueChildNameDefault()).
    gUniqueChildNameFn = fn;
}

std::string uniqueChildName(const UsdPrim& usdParent, const std::string& name)
{
    return gUniqueChildNameFn ? gUniqueChildNameFn(usdParent, name)
                              : uniqueChildNameDefault(usdParent, name);
}

std::string uniqueChildNameDefault(const UsdPrim& usdParent, const std::string& name)
{
    if (!usdParent.IsValid())
        return std::string();

    TfToken::HashSet childrenNames;

    // The prim GetChildren method used the UsdPrimDefaultPredicate which includes
    // active prims. We also need the inactive ones.
    //
    // const Usd_PrimFlagsConjunction UsdPrimDefaultPredicate =
    //			UsdPrimIsActive && UsdPrimIsDefined &&
    //			UsdPrimIsLoaded && !UsdPrimIsAbstract;
    // Note: removed 'UsdPrimIsLoaded' from the predicate. When it is present the
    //		 filter doesn't properly return the inactive prims. UsdView doesn't
    //		 use loaded either in _computeDisplayPredicate().
    //
    // Note: our UsdHierarchy uses instance proxies, so we also use them here.
    for (auto child : usdParent.GetFilteredChildren(
             UsdTraverseInstanceProxies(UsdPrimIsDefined && !UsdPrimIsAbstract))) {
        childrenNames.insert(child.GetName());
    }
    std::string childName { name };
    if (childrenNames.find(TfToken(childName)) != childrenNames.end()) {
        childName = uniqueName(childrenNames, childName);
    }
    return childName;
}

SdfPath uniqueChildPath(const UsdStage& stage, const SdfPath& path)
{
    const UsdPrim     parentPrim = stage.GetPrimAtPath(path.GetParentPath());
    const std::string originalName = path.GetName();
    const std::string uniqueName = uniqueChildName(parentPrim, originalName);
    if (uniqueName == originalName)
        return path;

    return path.ReplaceName(TfToken(uniqueName));
}

void setDisplayMessageFn(const DisplayMessageFn fns[static_cast<int>(MessageType::nbTypes)])
{
    // Each of the display message functions is allowed to be null in which case
    // a default function will be used for each.
    for (int i = 0; i < static_cast<int>(MessageType::nbTypes); ++i) {
        gDisplayMessageFn[i] = fns[i];
    }
}

void displayMessage(MessageType type, const std::string& msg)
{
    // If we have an (optional) display message for the input type, use it.
    // Otherwise use the default TF_ ones provided by USD.
    auto messageFn = gDisplayMessageFn[static_cast<int>(MessageType::kInfo)];
    if (messageFn) {
        messageFn(msg);
    } else {
        switch (type) {
        case MessageType::kInfo: TF_STATUS(msg); break;
        case MessageType::kWarning: TF_WARN(msg); break;
        case MessageType::KError: TF_RUNTIME_ERROR(msg); break;
        default: break;
        }
    }
}

namespace {
// Do not expose that function. The input parameter does not provide enough information to
// distinguish between kEnum and kEnumString.
Ufe::Attribute::Type _UsdTypeToUfe(const SdfValueTypeName& usdType)
{
    // Map the USD type into UFE type.
    static const std::unordered_map<size_t, Ufe::Attribute::Type> sUsdTypeToUfe {
        { SdfValueTypeNames->Bool.GetHash(), Ufe::Attribute::kBool },           // bool
        { SdfValueTypeNames->Int.GetHash(), Ufe::Attribute::kInt },             // int32_t
        { SdfValueTypeNames->Float.GetHash(), Ufe::Attribute::kFloat },         // float
        { SdfValueTypeNames->Double.GetHash(), Ufe::Attribute::kDouble },       // double
        { SdfValueTypeNames->String.GetHash(), Ufe::Attribute::kString },       // std::string
        { SdfValueTypeNames->Token.GetHash(), Ufe::Attribute::kString },        // TfToken
        { SdfValueTypeNames->Int3.GetHash(), Ufe::Attribute::kInt3 },           // GfVec3i
        { SdfValueTypeNames->Float3.GetHash(), Ufe::Attribute::kFloat3 },       // GfVec3f
        { SdfValueTypeNames->Double3.GetHash(), Ufe::Attribute::kDouble3 },     // GfVec3d
        { SdfValueTypeNames->Color3f.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3f
        { SdfValueTypeNames->Color3d.GetHash(), Ufe::Attribute::kColorFloat3 }, // GfVec3d
#ifdef UFE_V4_FEATURES_AVAILABLE
        { SdfValueTypeNames->Asset.GetHash(), Ufe::Attribute::kFilename },      // SdfAssetPath
        { SdfValueTypeNames->Float2.GetHash(), Ufe::Attribute::kFloat2 },       // GfVec2f
        { SdfValueTypeNames->Float4.GetHash(), Ufe::Attribute::kFloat4 },       // GfVec4f
        { SdfValueTypeNames->Color4f.GetHash(), Ufe::Attribute::kColorFloat4 }, // GfVec4f
        { SdfValueTypeNames->Color4d.GetHash(), Ufe::Attribute::kColorFloat4 }, // GfVec4d
        { SdfValueTypeNames->Matrix3d.GetHash(), Ufe::Attribute::kMatrix3d },   // GfMatrix3d
        { SdfValueTypeNames->Matrix4d.GetHash(), Ufe::Attribute::kMatrix4d },   // GfMatrix4d
#endif
    };
    const auto iter = sUsdTypeToUfe.find(usdType.GetHash());
    if (iter != sUsdTypeToUfe.end()) {
        return iter->second;
    } else {
        static const std::unordered_map<std::string, Ufe::Attribute::Type> sCPPTypeToUfe {
            // There are custom Normal3f, Point3f types in USD. They can all be recognized by the
            // underlying CPP type and if there is a Ufe type that matches, use it.
            { "GfVec3i", Ufe::Attribute::kInt3 },   { "GfVec3d", Ufe::Attribute::kDouble3 },
            { "GfVec3f", Ufe::Attribute::kFloat3 },
#ifdef UFE_V4_FEATURES_AVAILABLE
            { "GfVec2f", Ufe::Attribute::kFloat2 }, { "GfVec4f", Ufe::Attribute::kFloat4 },
#endif
        };

        const auto iter = sCPPTypeToUfe.find(usdType.GetCPPTypeName());
        if (iter != sCPPTypeToUfe.end()) {
            return iter->second;
        } else {
            return Ufe::Attribute::kGeneric;
        }
    }
}
} // namespace

Ufe::Attribute::Type usdTypeToUfe(const SdrShaderPropertyConstPtr& shaderProperty)
{
    Ufe::Attribute::Type retVal = Ufe::Attribute::kInvalid;

    const SdfValueTypeName typeName = shaderProperty->GetTypeAsSdfType().first;
    if (typeName.GetHash() == SdfValueTypeNames->Token.GetHash()) {
        static const TokenToSdfTypeMap tokenTypeToSdfType
            = { { SdrPropertyTypes->Int, SdfValueTypeNames->Int },
                { SdrPropertyTypes->String, SdfValueTypeNames->String },
                { SdrPropertyTypes->Float, SdfValueTypeNames->Float },
                { SdrPropertyTypes->Color, SdfValueTypeNames->Color3f },
#if defined(USD_HAS_COLOR4_SDR_SUPPORT)
                { SdrPropertyTypes->Color4, SdfValueTypeNames->Color4f },
#endif
                { SdrPropertyTypes->Point, SdfValueTypeNames->Point3f },
                { SdrPropertyTypes->Normal, SdfValueTypeNames->Normal3f },
                { SdrPropertyTypes->Vector, SdfValueTypeNames->Vector3f },
                { SdrPropertyTypes->Matrix, SdfValueTypeNames->Matrix4d } };
        TokenToSdfTypeMap::const_iterator it
            = tokenTypeToSdfType.find(shaderProperty->GetTypeAsSdfType().second);
        if (it != tokenTypeToSdfType.end()) {
            retVal = _UsdTypeToUfe(it->second);
        } else {
#if PXR_VERSION < 2205
            // Pre-22.05 boolean inputs are special:
            if (shaderProperty->GetType() == SdfValueTypeNames->Bool.GetAsToken()) {
                retVal = _UsdTypeToUfe(SdfValueTypeNames->Bool);
            } else
#endif
                // There is no Matrix3d type in Sdr, so we need to infer it from Sdf until a fix
                // similar to what was done to booleans is submitted to USD. This also means that
                // there will be no default value for that type.
                if (shaderProperty->GetType() == SdfValueTypeNames->Matrix3d.GetAsToken()) {
                retVal = _UsdTypeToUfe(SdfValueTypeNames->Matrix3d);
            } else {
                retVal = Ufe::Attribute::kGeneric;
            }
        }
    } else {
        retVal = _UsdTypeToUfe(typeName);
    }

    if (retVal == Ufe::Attribute::kString) {
        if (!shaderProperty->GetOptions().empty()) {
            retVal = Ufe::Attribute::kEnumString;
        }
#ifdef UFE_V4_FEATURES_AVAILABLE
        else if (shaderProperty->IsAssetIdentifier()) {
            retVal = Ufe::Attribute::kFilename;
        }
#endif
    }

    return retVal;
}

Ufe::Attribute::Type usdTypeToUfe(const PXR_NS::UsdAttribute& usdAttr)
{
    if (usdAttr.IsValid()) {
        const SdfValueTypeName typeName = usdAttr.GetTypeName();
        Ufe::Attribute::Type   type = _UsdTypeToUfe(typeName);
        if (type == Ufe::Attribute::kString) {
            // Both std::string and TfToken resolve to kString, but if there is a list of allowed
            // tokens, then we use kEnumString instead.
            if (usdAttr.GetPrim().GetPrimDefinition().GetPropertyMetadata<VtTokenArray>(
                    usdAttr.GetName(), SdfFieldKeys->AllowedTokens, nullptr)) {
                type = Ufe::Attribute::kEnumString;
            }
            UsdShadeNodeGraph asNodeGraph(usdAttr.GetPrim());
            if (asNodeGraph) {
                // NodeGraph inputs can have enum metadata on them when they export an inner enum.
                const auto portType = UsdShadeUtils::GetBaseNameAndType(usdAttr.GetName()).second;
                if (portType == UsdShadeAttributeType::Input) {
                    const auto input = UsdShadeInput(usdAttr);
                    if (!input.GetSdrMetadataByKey(TfToken("enum")).empty()) {
                        return Ufe::Attribute::kEnumString;
                    }
                }
                // TfToken is also used in UsdShade as a Generic placeholder for connecting struct
                // I/O.
                if (usdAttr.GetTypeName() == SdfValueTypeNames->Token
                    && portType != UsdShadeAttributeType::Invalid) {
                    type = Ufe::Attribute::kGeneric;
                }
            }
        }
        return type;
    }

    TF_RUNTIME_ERROR("Invalid USDAttribute: %s", usdAttr.GetPath().GetAsString().c_str());
    return Ufe::Attribute::kInvalid;
}

SdfValueTypeName ufeTypeToUsd(const Ufe::Attribute::Type ufeType)
{
    // Map the USD type into UFE type.
    static const std::unordered_map<Ufe::Attribute::Type, SdfValueTypeName> sUfeTypeToUsd {
        { Ufe::Attribute::kBool, SdfValueTypeNames->Bool },
        { Ufe::Attribute::kInt, SdfValueTypeNames->Int },
        { Ufe::Attribute::kFloat, SdfValueTypeNames->Float },
        { Ufe::Attribute::kDouble, SdfValueTypeNames->Double },
        { Ufe::Attribute::kString, SdfValueTypeNames->String },
        // Not enough info at this point to differentiate between TfToken and std:string.
        { Ufe::Attribute::kEnumString, SdfValueTypeNames->Token },
        { Ufe::Attribute::kInt3, SdfValueTypeNames->Int3 },
        { Ufe::Attribute::kFloat3, SdfValueTypeNames->Float3 },
        { Ufe::Attribute::kDouble3, SdfValueTypeNames->Double3 },
        { Ufe::Attribute::kColorFloat3, SdfValueTypeNames->Color3f },
        { Ufe::Attribute::kGeneric, SdfValueTypeNames->Token },
#ifdef UFE_V4_FEATURES_AVAILABLE
        { Ufe::Attribute::kFilename, SdfValueTypeNames->Asset },
        { Ufe::Attribute::kFloat2, SdfValueTypeNames->Float2 },
        { Ufe::Attribute::kFloat4, SdfValueTypeNames->Float4 },
        { Ufe::Attribute::kColorFloat4, SdfValueTypeNames->Color4f },
        { Ufe::Attribute::kMatrix3d, SdfValueTypeNames->Matrix3d },
        { Ufe::Attribute::kMatrix4d, SdfValueTypeNames->Matrix4d },
#endif
    };

    const auto iter = sUfeTypeToUsd.find(ufeType);
    if (iter != sUfeTypeToUsd.end()) {
        return iter->second;
    } else {
        return SdfValueTypeName();
    }
}

UsdAttribute* usdAttrFromUfeAttr(const Ufe::Attribute::Ptr& attr)
{
    if (!attr) {
        TF_RUNTIME_ERROR("Invalid attribute.");
        return nullptr;
    }

    if (attr->sceneItem()->runTimeId() != getUsdRunTimeId()) {
        TF_RUNTIME_ERROR(
            "Invalid runtime identifier for the attribute '" + attr->name() + "' in the node '"
            + Ufe::PathString::string(attr->sceneItem()->path()) + "'.");
        return nullptr;
    }

    return dynamic_cast<UsdAttribute*>(attr.get());
}

Ufe::Attribute::Ptr attrFromUfeAttrInfo(const Ufe::AttributeInfo& attrInfo)
{
    auto item
        = std::dynamic_pointer_cast<UsdSceneItem>(Ufe::Hierarchy::createItem(attrInfo.path()));
    if (!item) {
        TF_RUNTIME_ERROR("Invalid scene item.");
        return nullptr;
    }
    return UsdAttributes(item).attribute(attrInfo.name());
}

namespace {

bool allowedInStrongerLayer(
    const UsdPrim&                          prim,
    const SdfPrimSpecHandleVector&          primStack,
    const std::set<PXR_NS::SdfLayerRefPtr>& sessionLayers,
    bool                                    allowStronger)
{
    // If the flag to allow edits in a stronger layer if off, then it is not allowed.
    if (!allowStronger)
        return false;

    // If allowed, verify if the target layer is stronger than any existing layer with an opinion.
    auto stage = prim.GetStage();
    auto targetLayer = stage->GetEditTarget().GetLayer();
    auto topLayer = primStack.front()->GetLayer();

    const SdfLayerHandle searchRoot = isSessionLayer(targetLayer, sessionLayers)
        ? stage->GetSessionLayer()
        : stage->GetRootLayer();

    return getStrongerLayer(searchRoot, targetLayer, topLayer) == targetLayer;
}

} // namespace

Ufe::BBox3d combineUfeBBox(const Ufe::BBox3d& ufeBBox1, const Ufe::BBox3d& ufeBBox2)
{
    if (ufeBBox1.empty())
        return ufeBBox2;

    if (ufeBBox2.empty())
        return ufeBBox1;

    Ufe::BBox3d combinedBBox;

    combinedBBox.min.set(
        std::min(ufeBBox1.min.x(), ufeBBox2.min.x()),
        std::min(ufeBBox1.min.y(), ufeBBox2.min.y()),
        std::min(ufeBBox1.min.z(), ufeBBox2.min.z()));

    combinedBBox.max.set(
        std::max(ufeBBox1.max.x(), ufeBBox2.max.x()),
        std::max(ufeBBox1.max.y(), ufeBBox2.max.y()),
        std::max(ufeBBox1.max.z(), ufeBBox2.max.z()));

    return combinedBBox;
}

void applyRootLayerMetadataRestriction(const UsdPrim& prim, const std::string& commandName)
{
    // return early if prim is the pseudo-root.
    // this is a special case and could happen when one tries to drag a prim under the
    // proxy shape in outliner. Also note if prim is the pseudo-root, no def primSpec will be found.
    if (prim.IsPseudoRoot()) {
        return;
    }

    const auto stage = prim.GetStage();
    if (!stage)
        return;

    // If the target layer is the root layer, then the restrictions
    // do not apply since the edit target is on the layer that contains
    // the metadata.
    const SdfLayerHandle targetLayer = stage->GetEditTarget().GetLayer();
    const SdfLayerHandle rootLayer = stage->GetRootLayer();
    if (targetLayer == rootLayer)
        return;

    // Enforce the restriction that we cannot change the default prim
    // from a layer other than the root layer.
    if (prim == stage->GetDefaultPrim()) {
        const std::string layerName = rootLayer->GetDisplayName();
        const std::string err = TfStringPrintf(
            "Cannot %s [%s]. This prim is defined as the default prim on [%s]",
            commandName.c_str(),
            prim.GetName().GetString().c_str(),
            layerName.c_str());
        throw std::runtime_error(err.c_str());
    }
}

void applyRootLayerMetadataRestriction(
    const PXR_NS::UsdStageRefPtr& stage,
    const std::string&            commandName)
{
    if (!stage)
        return;

    // If the target layer is the root layer, then the restrictions
    // do not apply since the edit target is on the layer that contains
    // the metadata.
    const SdfLayerHandle targetLayer = stage->GetEditTarget().GetLayer();
    const SdfLayerHandle rootLayer = stage->GetRootLayer();
    if (targetLayer == rootLayer)
        return;

    // Enforce the restriction that we cannot change the default prim
    // from a layer other than the root layer.
    const std::string layerName = rootLayer->GetDisplayName();
    const std::string err = TfStringPrintf(
        "Cannot %s. The stage default prim metadata can only be modified when the root layer [%s] "
        "is targeted.",
        commandName.c_str(),
        layerName.c_str());
    throw std::runtime_error(err.c_str());
}

void applyCommandRestriction(
    const UsdPrim&     prim,
    const std::string& commandName,
    bool               allowStronger)
{
    // return early if prim is the pseudo-root.
    // this is a special case and could happen when one tries to drag a prim under the
    // proxy shape in outliner. Also note if prim is the pseudo-root, no def primSpec will be found.
    if (prim.IsPseudoRoot()) {
        return;
    }

    const auto stage = prim.GetStage();
    auto       targetLayer = stage->GetEditTarget().GetLayer();

    const bool includeTopLayer = true;
    const auto sessionLayers = getAllSublayerRefs(stage->GetSessionLayer(), includeTopLayer);
    const bool isTargetingSession = isSessionLayer(targetLayer, sessionLayers);

    auto        primSpec = getPrimSpecAtEditTarget(prim);
    auto        primStack = prim.GetPrimStack();
    std::string layerDisplayName;

    // When the command is forbidden even for the strongest layer, that means
    // that the operation is a multi-layers operation and there is no target
    // layer that would allow it to proceed. In that case, do not suggest changing
    // the target.
    std::string message = allowStronger ? "It is defined on another layer. " : "";
    std::string instructions = allowStronger ? "Please set %s as the target layer to proceed."
                                             : "It would orphan opinions on the layer %s";

    // iterate over the prim stack, starting at the highest-priority layer.
    for (const auto& spec : primStack) {
        // Only take session layers opinions into consideration when the target itself
        // is a session layer (top a sub-layer of session).
        //
        // We isolate session / non-session this way because these opinions
        // are owned by the application and we don't want to block the user
        // commands and user data due to them.
        const auto layer = spec->GetLayer();
        if (isSessionLayer(layer, sessionLayers) != isTargetingSession)
            continue;

        const auto& layerName = layer->GetDisplayName();

        // skip if there is no primSpec for the selected prim in the current stage's local layer.
        if (!primSpec) {
            // add "," separator for multiple layers
            if (!layerDisplayName.empty()) {
                layerDisplayName.append(",");
            }
            layerDisplayName.append("[" + layerName + "]");
            continue;
        }

        // one reason for skipping the references and payloads is to not clash
        // with the over that may be created in the stage's sessionLayer.
        // another reason is that one should be able to edit a referenced prim that
        // either as over/def as long as it has a primSpec in the selected edit target layer.
        if (spec->HasReferences() || spec->HasPayloads()) {
            break;
        }

        // if exists a def/over specs
        if (spec->GetSpecifier() == SdfSpecifierDef || spec->GetSpecifier() == SdfSpecifierOver) {
            // if spec exists in another layer ( e.g sessionLayer or layer other than stage's local
            // layers ).
            if (primSpec->GetLayer() != spec->GetLayer()) {
                layerDisplayName.append("[" + layerName + "]");
                if (allowStronger) {
                    message = "It has a stronger opinion on another layer. ";
                }
                break;
            }
            continue;
        }
    }

    // Per design request, we need a more clear message to indicate that renaming a prim inside a
    // variantset is not allowed. This restriction was already caught in the above loop but the
    // message was a bit generic.
    UsdPrimCompositionQuery query(prim);
    for (const auto& compQueryArc : query.GetCompositionArcs()) {
        if (!primSpec && PcpArcTypeVariant == compQueryArc.GetArcType()) {
            if (allowedInStrongerLayer(prim, primStack, sessionLayers, allowStronger))
                return;
            std::string err = TfStringPrintf(
                "Cannot %s [%s] because it is defined inside the variant composition arc %s",
                commandName.c_str(),
                prim.GetName().GetString().c_str(),
                layerDisplayName.c_str());
            throw std::runtime_error(err.c_str());
        }
    }

    if (!layerDisplayName.empty()) {
        if (allowedInStrongerLayer(prim, primStack, sessionLayers, allowStronger))
            return;
        std::string formattedInstructions
            = TfStringPrintf(instructions.c_str(), layerDisplayName.c_str());
        std::string err = TfStringPrintf(
            "Cannot %s [%s]. %s%s",
            commandName.c_str(),
            prim.GetName().GetString().c_str(),
            message.c_str(),
            formattedInstructions.c_str());
        throw std::runtime_error(err.c_str());
    }

    applyRootLayerMetadataRestriction(prim, commandName);
}

bool applyCommandRestrictionNoThrow(
    const UsdPrim&     prim,
    const std::string& commandName,
    bool               allowStronger)
{
    try {
        applyCommandRestriction(prim, commandName, allowStronger);
    } catch (const std::exception& e) {
        std::string errMsg(e.what());
        TF_WARN(errMsg);
        return false;
    }
    return true;
}

bool isPrimMetadataEditAllowed(
    const UsdPrim&         prim,
    const TfToken&         metadataName,
    const PXR_NS::TfToken& keyPath,
    std::string*           errMsg)
{
    return isPropertyMetadataEditAllowed(prim, TfToken(), metadataName, keyPath, errMsg);
}

bool isPropertyMetadataEditAllowed(
    const UsdPrim&         prim,
    const PXR_NS::TfToken& propName,
    const TfToken&         metadataName,
    const PXR_NS::TfToken& keyPath,
    std::string*           errMsg)
{
    // If the intended target layer is not modifiable as a whole,
    // then no metadata edits are allowed at all.
    const UsdStagePtr& stage = prim.GetStage();
    if (!UsdUfe::isEditTargetLayerModifiable(stage, errMsg))
        return false;

    // Find the highest layer that has the metadata authored. The prim
    // expanded PCP index, which contains all locations that contribute
    // to the prim, is scanned for the first metadata authoring.
    //
    // Note: as far as we know, there are no USD API to retrieve the list
    //       of authored locations for a metadata, unlike properties.
    //
    //       The code here is inspired by code from USD, according to
    //       the following call sequence:
    //          - UsdObject::GetAllAuthoredMetadata()
    //          - UsdStage::_GetAllMetadata()
    //          - UsdStage::_GetMetadataImpl()
    //          - UsdStage::_GetGeneralMetadataImpl()
    //          - Usd_Resolver class
    //          - _ComposeGeneralMetadataImpl()
    //          - ExistenceComposer::ConsumeAuthored()
    //          - SdfLayer::HasFieldDictKey()
    //
    //          - UsdPrim::GetVariantSets
    //          - UsdVariantSet::GetVariantSelection()
    SdfLayerHandle topAuthoredLayer;
    {
        const SdfPath       primPath = prim.GetPath();
        const PcpPrimIndex& primIndex = prim.ComputeExpandedPrimIndex();

        // We need special processing for variant selection.
        //
        // Note: we would also need spacial processing for reference and payload,
        //       but let's postpone them until we actually need it since it would
        //       add yet more complexities.
        const bool isVariantSelection = (metadataName == SdfFieldKeys->VariantSelection);

        // Note: specPath is important even if prop name is empty, it then means
        //       a metadata on the prim itself.
        Usd_Resolver resolver(&primIndex);
        SdfPath      specPath = resolver.GetLocalPath(propName);

        for (bool isNewNode = false; resolver.IsValid(); isNewNode = resolver.NextLayer()) {
            if (isNewNode)
                specPath = resolver.GetLocalPath(propName);

            // Consume an authored opinion here, if one exists.
            SdfLayerRefPtr const& layer = resolver.GetLayer();
            const bool            gotOpinion = keyPath.IsEmpty() || isVariantSelection
                ? layer->HasField(specPath, metadataName)
                : layer->HasFieldDictKey(specPath, metadataName, keyPath);

            if (gotOpinion) {
                if (isVariantSelection) {
                    using SelMap = SdfVariantSelectionMap;
                    const SelMap variantSel = layer->GetFieldAs<SelMap>(specPath, metadataName);
                    if (variantSel.count(keyPath) == 0) {
                        continue;
                    }
                }
                topAuthoredLayer = layer;
                break;
            }
        }
    }

    // Get the layer where we intend to author a new opinion.
    const UsdEditTarget& editTarget = stage->GetEditTarget();
    const SdfLayerHandle targetLayer = editTarget.GetLayer();

    // Verify that the intended target layer is stronger than existing authored opinions.
    const auto strongestLayer
        = UsdUfe::getStrongerLayer(stage, targetLayer, topAuthoredLayer, true);
    bool allowed = (strongestLayer == targetLayer);
    if (!allowed && errMsg) {
        std::string strongName;
        if (strongestLayer)
            strongName = strongestLayer->GetDisplayName();
        else
            strongName = "a layer we could not identify";

        *errMsg = TfStringPrintf(
            "Cannot edit [%s] attribute because there is a stronger opinion in [%s].",
            metadataName.GetText(),
            strongName.c_str());
    }
    return allowed;
}

bool isAttributeEditAllowed(const PXR_NS::UsdAttribute& attr, std::string* errMsg)
{
    if (isAttributedLocked(attr, errMsg))
        return false;

    // get the property spec in the edit target's layer
    const auto& prim = attr.GetPrim();
    const auto& stage = prim.GetStage();
    const auto& editTarget = stage->GetEditTarget();

    if (!UsdUfe::isEditTargetLayerModifiable(stage, errMsg)) {
        return false;
    }

    // get the index to edit target layer
    const auto targetLayerIndex = findLayerIndex(prim, editTarget.GetLayer());

    // HS March 22th,2021
    // TODO: "Value Clips" are UsdStage-level feature, unknown to Pcp.So if the attribute in
    // question is affected by Value Clips, we would will likely get the wrong answer. See Spiff
    // comment for more information :
    // https://groups.google.com/g/usd-interest/c/xTxFYQA_bRs/m/lX_WqNLoBAAJ

    // Read on Value Clips here:
    // https://graphics.pixar.com/usd/docs/api/_usd__page__value_clips.html

    // get the strength-ordered ( strong-to-weak order ) list of property specs that provide
    // opinions for this property.
    const auto& propertyStack = attr.GetPropertyStack();

    if (!propertyStack.empty()) {
        // get the strongest layer that has the attr.
        auto strongestLayer = attr.GetPropertyStack().front()->GetLayer();

        // compare the calculated index between the "attr" and "edit target" layers.
        if (findLayerIndex(prim, strongestLayer) < targetLayerIndex) {
            if (errMsg) {
                *errMsg = TfStringPrintf(
                    "Cannot edit [%s] attribute because there is a stronger opinion in [%s].",
                    attr.GetBaseName().GetText(),
                    strongestLayer->GetDisplayName().c_str());
            }

            return false;
        }
    }

    return true;
}

bool isAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName, std::string* errMsg)
{
    TF_AXIOM(prim);
    TF_AXIOM(!attrName.IsEmpty());

    UsdGeomXformable xformable(prim);
    if (xformable) {
        if (UsdGeomXformOp::IsXformOp(attrName)) {
            // check for the attribute in XformOpOrderAttr first
            if (!isAttributeEditAllowed(xformable.GetXformOpOrderAttr(), errMsg)) {
                return false;
            }
        }
    }
    // check the attribute itself
    if (!isAttributeEditAllowed(prim.GetAttribute(attrName), errMsg)) {
        return false;
    }

    return true;
}

bool isAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName)
{
    std::string errMsg;
    if (!isAttributeEditAllowed(prim, attrName, &errMsg)) {
        TF_WARN(errMsg);
        return false;
    }

    return true;
}

void enforceAttributeEditAllowed(const PXR_NS::UsdAttribute& attr)
{
    std::string errMsg;
    if (!isAttributeEditAllowed(attr, &errMsg)) {
        TF_WARN(errMsg);
        throw std::runtime_error(errMsg);
    }
}

void enforceAttributeEditAllowed(const UsdPrim& prim, const TfToken& attrName)
{
    std::string errMsg;
    if (!isAttributeEditAllowed(prim, attrName, &errMsg)) {
        TF_WARN(errMsg);
        throw std::runtime_error(errMsg);
    }
}

bool isAnyLayerModifiable(const UsdStageWeakPtr stage, std::string* errMsg /* = nullptr */)
{
    PXR_NS::SdfLayerHandleVector layers = stage->GetLayerStack(false);
    for (auto layer : layers) {
        if (!layer->IsMuted() && layer->PermissionToEdit()) {
            return true;
        }
    }

    if (errMsg) {
        std::string err = TfStringPrintf(
            "Cannot target any layers in the stage [%s] because the layers are either locked or "
            "muted. Switching to session layer.",
            stage->GetRootLayer()->GetIdentifier().c_str());

        *errMsg = err;
    }

    return false;
}

bool isEditTargetLayerModifiable(const UsdStageWeakPtr stage, std::string* errMsg)
{
    const auto editTarget = stage->GetEditTarget();
    const auto editLayer = editTarget.GetLayer();

    if (editLayer && !editLayer->PermissionToEdit()) {
        if (errMsg) {
            std::string err = TfStringPrintf(
                "Cannot edit [%s] because it is locked. Unlock it to proceed.",
                editLayer->GetDisplayName().c_str());

            *errMsg = err;
        }

        return false;
    }

    if (stage->IsLayerMuted(editLayer->GetIdentifier())) {
        if (errMsg) {
            std::string err = TfStringPrintf(
                "Cannot edit [%s] because it is muted. Unmute [%s] to proceed.",
                editLayer->GetDisplayName().c_str(),
                editLayer->GetDisplayName().c_str());
            *errMsg = err;
        }

        return false;
    }

    return true;
}

Ufe::Selection removeDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath)
{
    // Filter the src selection, removing items below the filterPath
    Ufe::Selection dst;
    for (const auto& item : src) {
        const auto& itemPath = item->path();
        // The filterPath itself is still valid.
        if (!itemPath.startsWith(filterPath) || itemPath == filterPath) {
            dst.append(item);
        }
    }
    return dst;
}

Ufe::Selection recreateDescendants(const Ufe::Selection& src, const Ufe::Path& filterPath)
{
    // If a src selection item starts with the filterPath, re-create it.
    Ufe::Selection dst;
    for (const auto& item : src) {
        const auto& itemPath = item->path();
        // The filterPath itself is still valid.
        if (!itemPath.startsWith(filterPath) || itemPath == filterPath) {
            dst.append(item);
        } else {
            auto recreatedItem = Ufe::Hierarchy::createItem(item->path());
            if (recreatedItem)
                dst.append(recreatedItem);
        }
    }
    return dst;
}

#ifdef UFE_SCENEITEM_HAS_METADATA
PXR_NS::VtValue ufeValueToVtValue(const Ufe::Value& ufeValue)
{
    PXR_NS::VtValue usdValue;
    if (ufeValue.isType<bool>())
        usdValue = ufeValue.get<bool>();
    else if (ufeValue.isType<int>())
        usdValue = ufeValue.get<int>();
    else if (ufeValue.isType<float>())
        usdValue = ufeValue.get<float>();
    else if (ufeValue.isType<double>())
        usdValue = ufeValue.get<double>();
    else if (ufeValue.isType<std::string>())
        usdValue = ufeValue.get<std::string>();
    else {
        TF_CODING_ERROR(kErrorMsgInvalidValueType);
    }

    return usdValue;
}

Ufe::Value vtValueToUfeValue(const PXR_NS::VtValue& vtValue)
{
    if (vtValue.IsHolding<bool>())
        return Ufe::Value(vtValue.Get<bool>());
    else if (vtValue.IsHolding<int>())
        return Ufe::Value(vtValue.Get<int>());
    else if (vtValue.IsHolding<float>())
        return Ufe::Value(vtValue.Get<float>());
    else if (vtValue.IsHolding<double>())
        return Ufe::Value(vtValue.Get<double>());
    else if (vtValue.IsHolding<std::string>())
        return Ufe::Value(vtValue.Get<std::string>());
    else if (vtValue.IsHolding<PXR_NS::TfToken>())
        return Ufe::Value(vtValue.Get<PXR_NS::TfToken>().GetString());
    else {
        std::stringstream ss;
        ss << vtValue;
        return Ufe::Value(ss.str());
    }
}
#endif

PXR_NS::SdrShaderNodeConstPtr usdShaderNodeFromSceneItem(const Ufe::SceneItem::Ptr& item)
{
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    PXR_NAMESPACE_USING_DIRECTIVE
    if (!TF_VERIFY(usdItem)) {
        return nullptr;
    }
    PXR_NS::UsdPrim        prim = usdItem->prim();
    PXR_NS::UsdShadeShader shader(prim);
    if (!shader) {
        return nullptr;
    }
    PXR_NS::TfToken mxNodeType;
    shader.GetIdAttr().Get(&mxNodeType);

    // Careful around name and identifier. They are not the same concept.
    //
    // Here is one example from MaterialX to illustrate:
    //
    //  ND_standard_surface_surfaceshader exists in 2 versions with identifiers:
    //     ND_standard_surface_surfaceshader     (latest version)
    //     ND_standard_surface_surfaceshader_100 (version 1.0.0)
    // Same name, 2 different identifiers.
    PXR_NS::SdrRegistry& registry = PXR_NS::SdrRegistry::GetInstance();
    return registry.GetShaderNodeByIdentifier(mxNodeType);
}

void setWaitCursorFns(WaitCursorFn startFn, WaitCursorFn stopFn)
{
    gStartWaitCursorFn = startFn;
    gStopWaitCursorFn = stopFn;
}

void startWaitCursor()
{
    if (!gStartWaitCursorFn)
        return;

    if (gWaitCursorCount == 0)
        gStartWaitCursorFn();

    ++gWaitCursorCount;
}

void stopWaitCursor()
{
    if (!gStopWaitCursorFn)
        return;

    --gWaitCursorCount;

    if (gWaitCursorCount == 0)
        gStopWaitCursorFn();
}

} // namespace USDUFE_NS_DEF
