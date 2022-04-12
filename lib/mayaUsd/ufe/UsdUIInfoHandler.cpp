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

#include "UsdSceneItem.h"

#include <pxr/usd/sdf/listOp.h> // SdfReferenceListOp/SdfPayloadListOp/SdfPathListOp
#include <pxr/usd/sdf/schema.h> // SdfFieldKeys
#include <pxr/usd/usd/variantSets.h>

#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>

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
        tooltip += "<b>Introduced Composition Arcs:</b> ";
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

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdUIInfoHandler::UsdUIInfoHandler()
    : Ufe::UIInfoHandler()
{
    // Register a callback to invalidate the invisible color.
    fColorChangedCallbackId = MEventMessage::addEventCallback(
        "DisplayRGBColorChanged", onColorChanged, reinterpret_cast<void*>(this));

    // Immediately update the invisible color to get a starting current value.
    updateInvisibleColor();
}

UsdUIInfoHandler::~UsdUIInfoHandler()
{
    // Unregister the callback used to invalidate the invisible color.
    if (fColorChangedCallbackId)
        MMessage::removeCallback(fColorChangedCallbackId);
}

/*static*/
UsdUIInfoHandler::Ptr UsdUIInfoHandler::create() { return std::make_shared<UsdUIInfoHandler>(); }

void UsdUIInfoHandler::updateInvisibleColor()
{
    // Retrieve the invisible color of the outliner.
    //
    // We *cannot* intialize it in treeViewCellInfo() because
    // that function gets called in a paint event and calling
    // a command in a painting event can cause a recursive paint
    // event if commands echoing is on, which can corrupt the
    // Qt paint internal which lead to a crash. Typical symptom
    // is that the state variable of the Qt paint engine becomes
    // null midway through the repaint.

    MDoubleArray color;
    MGlobal::executeCommand("displayRGBColor -q \"outlinerInvisibleColor\"", color);

    if (color.length() >= 3) {
        color.get(fInvisibleColor.data());
        fInvisibleColorValid = true;
    }
}

/*static*/
void UsdUIInfoHandler::onColorChanged(void* data)
{
    UsdUIInfoHandler* infoHandler = reinterpret_cast<UsdUIInfoHandler*>(data);
    if (!infoHandler)
        return;

    infoHandler->updateInvisibleColor();
}

//------------------------------------------------------------------------------
// Ufe::UIInfoHandler overrides
//------------------------------------------------------------------------------

bool UsdUIInfoHandler::treeViewCellInfo(const Ufe::SceneItem::Ptr& item, Ufe::CellInfo& info) const
{
    bool              changed = false;
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
#if !defined(NDEBUG)
    assert(usdItem);
#endif
    if (usdItem) {
        if (!usdItem->prim().IsActive()) {
            changed = true;
            info.fontStrikeout = true;
            if (fInvisibleColorValid) {
                info.textFgColor.set(
                    static_cast<float>(fInvisibleColor[0]),
                    static_cast<float>(fInvisibleColor[1]),
                    static_cast<float>(fInvisibleColor[2]));
            } else {
                info.textFgColor.set(0.403922f, 0.403922f, 0.403922f);
            }
        }
    }

    return changed;
}

Ufe::UIInfoHandler::Icon UsdUIInfoHandler::treeViewIcon(const Ufe::SceneItem::Ptr& item) const
{
    // Special case for nullptr input.
    if (!item) {
        return Ufe::UIInfoHandler::Icon("out_USD_UsdTyped.png"); // Default USD icon
    }

    // We support these node types directly.
    static const std::map<std::string, std::string> supportedTypes {
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
        { "MayaReference", "out_USD_MayaReference.png" },
        { "ALMayaReference", "out_USD_MayaReference.png" }, // Same as mayaRef
        { "Mesh", "out_USD_Mesh.png" },
        { "NurbsPatch", "out_USD_NurbsPatch.png" },
        { "PointInstancer", "out_USD_PointInstancer.png" },
        { "Points", "out_USD_Points.png" },
        { "Scope", "out_USD_Scope.png" },
        { "SkelAnimation", "out_USD_SkelAnimation.png" },
        { "Skeleton", "out_USD_Skeleton.png" },
        { "SkelRoot", "out_USD_SkelRoot.png" },
        { "Sphere", "out_USD_Sphere.png" },
        { "Volume", "out_USD_Volume.png" }
    };

    Ufe::UIInfoHandler::Icon icon; // Default is empty (no icon and no badge).
    const auto               search = supportedTypes.find(item->nodeType());
    if (search != supportedTypes.cend()) {
        icon.baseIcon = search->second;
    }

    // Check if we have any composition meta data - if yes we display a special badge.
    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (usdItem) {
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

    UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
    if (usdItem) {
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

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
