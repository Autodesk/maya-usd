//
// Copyright 2020 Autodesk
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
#include "UsdUIInfoHandler.h"

#include <usdUfe/ufe/UsdSceneItem.h>
#include <usdUfe/ufe/Utils.h>

#include <pxr/usd/sdf/listOp.h> // SdfReferenceListOp/SdfPayloadListOp/SdfPathListOp
#include <pxr/usd/sdf/schema.h> // SdfFieldKeys
#include <pxr/usd/usd/variantSets.h>

#include <map>
#include <vector>

namespace {
// Simple helper to add the metadata strings to the end of the input tooltip string.
// Depending on the count, will add singular string or plural (with count).
void addMetadataStrings(
    const int          nb,
    std::string&       tooltip,
    bool&              needComma,
    const std::string& singular,
    const std::string& plural)
{
    if (nb <= 0)
        return;
    if (tooltip.empty())
        tooltip += "<b>Composition Arcs:</b> ";
    if (needComma)
        tooltip += ", ";
    if (nb == 1) {
        tooltip += singular;
    } else {
        tooltip += std::to_string(nb);
        tooltip += " ";
        tooltip += plural;
    }
    needComma = true;
}

// Simple template helper function to handle all the various types of listOps.
template <typename T>
void addMetadataCount(
    const T&           op,
    std::string&       tooltip,
    bool&              needComma,
    const std::string& singular,
    const std::string& plural)
{
    typename T::ItemVector refs;
    op.ApplyOperations(&refs);
    if (!refs.empty()) {
        addMetadataStrings(refs.size(), tooltip, needComma, singular, plural);
    }
}

} // namespace

namespace USDUFE_NS_DEF {

USDUFE_VERIFY_CLASS_SETUP(Ufe::UIInfoHandler, UsdUIInfoHandler);

UsdUIInfoHandler::UsdUIInfoHandler()
    : Ufe::UIInfoHandler()
{
    // Initialize to invalid values.
    fInvisibleColor[0] = -1.;
    fInvisibleColor[1] = -1.;
    fInvisibleColor[2] = -1.;
}

/*static*/
UsdUIInfoHandler::Ptr UsdUIInfoHandler::create() { return std::make_shared<UsdUIInfoHandler>(); }

//------------------------------------------------------------------------------
// Ufe::UIInfoHandler overrides
//------------------------------------------------------------------------------

bool UsdUIInfoHandler::treeViewCellInfo(const Ufe::SceneItem::Ptr& item, Ufe::CellInfo& info) const
{
    bool              changed = false;
    UsdSceneItem::Ptr usdItem = downcast(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif
    if (usdItem && usdItem->prim()) {
        if (!usdItem->prim().IsActive()) {
            changed = true;
            info.fontStrikeout = true;
            if (fInvisibleColor[0] >= 0) {
                info.textFgColor.set(
                    static_cast<float>(fInvisibleColor[0]),
                    static_cast<float>(fInvisibleColor[1]),
                    static_cast<float>(fInvisibleColor[2]));
            } else {
                // Default color (dark gray) if none provided.
                info.textFgColor.set(0.403922f, 0.403922f, 0.403922f);
            }
        }
    }

    return changed;
}

UsdUIInfoHandler::SupportedTypesMap UsdUIInfoHandler::getSupportedIconTypes() const
{
    // We support these node types directly.
    static const SupportedTypesMap supportedTypes {
        { "", "out_USD_Def.png" }, // No node type
        { "BlendShape", "out_USD_BlendShape.png" },
        { "Camera", "out_USD_Camera.png" },
        { "Capsule", "out_USD_Capsule.png" },
        { "Cone", "out_USD_Cone.png" },
        { "Cube", "out_USD_Cube.png" },
        { "Cylinder", "out_USD_Cylinder.png" },
        { "GeomSubset", "out_USD_GeomSubset.png" },
        { "LightFilter", "out_USD_LightFilter.png" },
        { "LightPortal", "out_USD_LightPortal.png" },
        { "Mesh", "out_USD_Mesh.png" },
        { "NurbsPatch", "out_USD_NurbsPatch.png" },
        { "PluginLight", "out_USD_PluginLight.png" },
        { "PointInstancer", "out_USD_PointInstancer.png" },
        { "Points", "out_USD_Points.png" },
        { "Scope", "out_USD_Scope.png" },
        { "SkelAnimation", "out_USD_SkelAnimation.png" },
        { "Skeleton", "out_USD_Skeleton.png" },
        { "SkelRoot", "out_USD_SkelRoot.png" },
        { "Sphere", "out_USD_Sphere.png" },
        { "Volume", "out_USD_Volume.png" },
        { "Material", "out_USD_Material.png" },
        { "NodeGraph", "out_USD_Shader.png" },
        { "Shader", "out_USD_Shader.png" },
    };
    return supportedTypes;
}

Ufe::UIInfoHandler::Icon UsdUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& item) const
{
    // Special case for nullptr input.
    if (!item) {
        return Ufe::UIInfoHandler::Icon("out_USD_UsdTyped.png"); // Default USD icon
    }

    auto                     supportedTypes = getSupportedIconTypes();
    Ufe::UIInfoHandler::Icon icon; // Default is empty (no icon and no badge).
    const auto               search = supportedTypes.find(item->nodeType());
    if (search != supportedTypes.cend()) {
        icon.baseIcon = search->second;
    }

    // Check if we have any composition meta data - if yes we display a special badge.
    auto usdItem = downcast(item);
    if (usdItem && usdItem->prim()) {
        // Variants
        if (!usdItem->prim().GetVariantSets().GetNames().empty()) {
            icon.badgeIcon = "out_USD_CompArcBadgeV.png";
            icon.pos = Ufe::UIInfoHandler::LowerRight;
        } else {
            // Composition related metadata.
            static const std::vector<PXR_NS::TfToken> compKeys
                = { PXR_NS::SdfFieldKeys->References,
                    PXR_NS::SdfFieldKeys->Payload,
                    PXR_NS::SdfFieldKeys->InheritPaths,
                    PXR_NS::SdfFieldKeys->Specializes };
            for (const auto& k : compKeys) {
                if (usdItem->prim().HasMetadata(k)) {
                    icon.badgeIcon = "out_USD_CompArcBadge.png";
                    icon.pos = Ufe::UIInfoHandler::LowerRight;
                    break;
                }
            }
        }
    }

    return icon;
}

std::string UsdUIInfoHandler::treeViewTooltip(const Ufe::SceneItem::Ptr& item) const
{
    std::string tooltip;

    auto usdItem = downcast(item);
    if (usdItem && usdItem->prim()) {
        // Composition related metadata.
        bool                       needComma = false;
        PXR_NS::SdfReferenceListOp referenceOp;
        if (usdItem->prim().GetMetadata(PXR_NS::SdfFieldKeys->References, &referenceOp)) {
            addMetadataCount<PXR_NS::SdfReferenceListOp>(
                referenceOp, tooltip, needComma, "Reference", "References");
        }

        PXR_NS::SdfPayloadListOp payloadOp;
        if (usdItem->prim().GetMetadata(PXR_NS::SdfFieldKeys->Payload, &payloadOp)) {
            addMetadataCount<PXR_NS::SdfPayloadListOp>(
                payloadOp, tooltip, needComma, "Payload", "Payloads");
        }

        PXR_NS::SdfPathListOp inheritOp;
        if (usdItem->prim().GetMetadata(PXR_NS::SdfFieldKeys->InheritPaths, &inheritOp)) {
            addMetadataCount<PXR_NS::SdfPathListOp>(
                inheritOp, tooltip, needComma, "Inherit", "Inherits");
        }

        PXR_NS::SdfPathListOp specializeOp;
        if (usdItem->prim().GetMetadata(PXR_NS::SdfFieldKeys->Specializes, &specializeOp)) {
            addMetadataCount<PXR_NS::SdfPathListOp>(
                specializeOp, tooltip, needComma, "Specialize", "Specializes");
        }

        // Variants
        const auto& variants = usdItem->prim().GetVariantSets().GetNames();
        if (!variants.empty()) {
            addMetadataStrings(variants.size(), tooltip, needComma, "Variant", "Variants");
        }
    }
    return tooltip;
}

std::string UsdUIInfoHandler::getLongRunTimeLabel() const { return "Universal Scene Description"; }

} // namespace USDUFE_NS_DEF
