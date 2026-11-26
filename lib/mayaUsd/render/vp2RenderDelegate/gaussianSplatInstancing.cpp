//
// Copyright 2026 Autodesk
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
// Instancing of HdVP2GaussianSplat: instance matrices, display-layer filtering, per-instance
// tints and UFE identifiers.
//
#include "gaussianSplatInstancing.h"

#if PXR_VERSION >= 2605

#include "gaussianSplat.h"
#include "instancer.h"

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/tokens.h>

namespace MAYAUSD_NS_DEF {

namespace {

/*! \brief  Tint of each USD instance: 0 none, 1 active, 2 lead.

    A fully selected prim tints all its instances. Otherwise only the instances of the selection
    states do. Empty when the prim is not selected.
*/
std::vector<unsigned char> _GetInstanceTintKinds(
    const PXR_NS::HdVP2SelectionStatus selectionStatus,
    PXR_NS::ProxyRenderDelegate&       drawScene,
    const PXR_NS::SdfPath&             id,
    const unsigned int                 instanceCount)
{
    std::vector<unsigned char> tintKind;
    if (selectionStatus == PXR_NS::kUnselected) {
        return tintKind;
    }

    const unsigned char all = selectionStatus == PXR_NS::kFullyLead
        ? 2
        : (selectionStatus == PXR_NS::kFullyActive ? 1 : 0);
    tintKind.assign(instanceCount, all);
    if (selectionStatus == PXR_NS::kPartiallySelected) {
        const auto mark
            = [&](const PXR_NS::HdSelection::PrimSelectionState* state, unsigned char kind) {
                  if (!state) {
                      return;
                  }
                  for (const auto& indexArray : state->instanceIndices) {
                      for (const auto index : indexArray) {
                          // Ids are positions in the visible list of the instancer. The scene index
                          // selection of a whole instancer also counts instances hidden by
                          // invisibleIds, skip those ids.
                          if (index >= 0 && index < static_cast<int>(instanceCount)) {
                              tintKind[index] = kind;
                          }
                      }
                  }
              };
        mark(drawScene.GetActiveSelectionState(id), 1);
        mark(drawScene.GetLeadSelectionState(id), 2);
    }
    return tintKind;
}

} // anonymous namespace

/*! \brief  Build the instancing state of a draw item. Worker thread.

    The instancer gives one matrix per instance, without the transform of the prototype. They are
    pulled each time, because the instancer dirty bits do not tell what changed. An instance whose
    display layer forces a wireframe, untextured or bounding box draw is dropped (the splat has no
    forced reprs), from the matrices and from the tints. Nothing is committed when the matrices
    and the tints are the ones the item already draws.
*/
HdVP2GaussianSplatInstancing HdVP2GaussianSplat::_UpdateInstancing(
    PXR_NS::HdSceneDelegate*    sceneDelegate,
    PXR_NS::HdVP2DrawItem*      drawItem,
    const PXR_NS::TfToken&      reprToken,
    const bool                  isWire,
    PXR_NS::MayaUsdCommitState& stateToCommit)
{
    HdVP2GaussianSplatInstancing           instancing;
    PXR_NS::HdVP2DrawItem::RenderItemData& drawItemData = stateToCommit._renderItemData;

    instancing._instanced = !GetInstancerId().IsEmpty();
    if (!instancing._instanced) {
        return instancing;
    }

    const PXR_NS::SdfPath& id = GetId();
    auto* const param = static_cast<PXR_NS::HdVP2RenderParam*>(_delegate->GetRenderParam());
    PXR_NS::ProxyRenderDelegate& drawScene = param->GetDrawScene();
    const MMatrix&               worldMatrix = drawItemData._worldMatrix;
    const PXR_NS::HdDirtyBits    itemDirtyBits = drawItem->GetDirtyBits();

    PXR_NS::HdRenderIndex&  renderIndex = sceneDelegate->GetRenderIndex();
    PXR_NS::HdVP2Instancer* instancer
        = static_cast<PXR_NS::HdVP2Instancer*>(renderIndex.GetInstancer(GetInstancerId()));
    const PXR_NS::VtMatrix4dArray transforms
        = instancer ? instancer->GetInstanceTransforms(id) : PXR_NS::VtMatrix4dArray();
    const unsigned int instanceCount = static_cast<unsigned int>(transforms.size());

    stateToCommit._instanceTransforms = std::make_shared<MMatrixArray>();
    instancing._tints = std::make_shared<MFloatArray>();
    instancing._mayaToUsdIds = std::make_shared<std::vector<unsigned int>>();
    MFloatArray& instanceTints = *instancing._tints;

    if (instanceCount > 0) {
        if (itemDirtyBits
            & (PXR_NS::MayaUsdRPrim::DirtyDisplayLayers
               | PXR_NS::HdChangeTracker::DirtyInstanceIndex
               | PXR_NS::HdChangeTracker::DirtyInstancer)) {
            _SyncDisplayLayerModesInstanced(id, instanceCount);
        }

        // Wire items keep the splat color.
        const std::vector<unsigned char> tintKind = isWire
            ? std::vector<unsigned char>()
            : _GetInstanceTintKinds(_selectionStatus, drawScene, id, instanceCount);

        const MColor tintColors[3]
            = { MColor(),
                _GetHighlightColor(PXR_NS::HdPrimTypeTokens->points, PXR_NS::kFullyActive),
                _GetHighlightColor(PXR_NS::HdPrimTypeTokens->points, PXR_NS::kFullyLead) };

        const PXR_NS::SdfPathVector usdPaths = drawScene.GetScenePrimPaths(id, instanceCount);
        const int                   modFlags = drawItem->GetModFlags();
        InstanceColorOverride       colorOverride(false);

        MMatrix instanceMatrix;
        stateToCommit._instanceTransforms->setLength(instanceCount);
        instanceTints.setLength(isWire ? 0 : instanceCount * 4);
        instancing._mayaToUsdIds->reserve(instanceCount);
        unsigned int mayaIndex = 0;
        for (unsigned int usdInstanceId = 0; usdInstanceId < instanceCount; ++usdInstanceId) {
            // An instance whose display layer forces a wireframe, untextured or bounding box
            // draw is not drawn (the splat has no forced reprs), nor is a hidden one.
            BasicWireframeColors info = kDormant;
            colorOverride.Reset();
            if (_FilterInstanceByDisplayLayer(
                    usdInstanceId,
                    info,
                    reprToken,
                    modFlags,
                    /*isHighlightItem*/ false,
                    /*isDedicatedHighlightItem*/ false,
                    colorOverride)) {
                continue;
            }

            transforms[usdInstanceId].Get(instanceMatrix.matrix);
            (*stateToCommit._instanceTransforms)[mayaIndex] = worldMatrix * instanceMatrix;
            stateToCommit._ufeIdentifiers.append(usdPaths[usdInstanceId].GetString().c_str());
            instancing._mayaToUsdIds->push_back(usdInstanceId);

            if (!isWire) {
                const unsigned char kind = tintKind.empty() ? 0 : tintKind[usdInstanceId];
                const MColor&       color = tintColors[kind];
                float* const        out = &instanceTints[mayaIndex * 4];
                out[0] = color.r;
                out[1] = color.g;
                out[2] = color.b;
                out[3] = kind == 0 ? 0.0f : kHdVP2GaussianSplatSelectionTintAlpha;
            }
            ++mayaIndex;
        }
        stateToCommit._instanceTransforms->setLength(mayaIndex);
        if (!isWire) {
            instanceTints.setLength(mayaIndex * 4);
        }
        if (mayaIndex == instanceCount) {
            // No instance dropped: the Maya instance index is the USD one.
            instancing._mayaToUsdIds->clear();
        }
        if (mayaIndex == 0) {
            instancing._noInstances = true;
        }
    } else {
        instancing._noInstances = true;
    }

    // Nothing to commit when the matrices are the ones the item already draws.
    bool transformsChanged = !drawItemData._instanceTransforms
        || drawItemData._instanceTransforms->length()
            != stateToCommit._instanceTransforms->length();
    for (unsigned int i = 0; !transformsChanged && i < stateToCommit._instanceTransforms->length();
         ++i) {
        transformsChanged
            = (*stateToCommit._instanceTransforms)[i] != (*drawItemData._instanceTransforms)[i];
    }
    if (!transformsChanged) {
        stateToCommit._instanceTransforms.reset();
        stateToCommit._ufeIdentifiers.clear();
    } else {
        drawItemData._instanceTransforms = stateToCommit._instanceTransforms;
    }

    // Same for the tints.
    if (!isWire) {
        instancing._tintsChanged = !drawItemData._instanceColors
            || drawItemData._instanceColors->length() != instanceTints.length();
        for (unsigned int i = 0; !instancing._tintsChanged && i < instanceTints.length(); ++i) {
            instancing._tintsChanged = instanceTints[i] != (*drawItemData._instanceColors)[i];
        }
        if (instancing._tintsChanged) {
            drawItemData._instanceColors = instancing._tints;
        }
    }

    return instancing;
}

void HdVP2GaussianSplatCommitInstancing(
    PXR_NS::ProxyRenderDelegate&        drawScene,
    MHWRender::MRenderItem&             renderItem,
    const PXR_NS::MayaUsdCommitState&   stateToCommit,
    const HdVP2GaussianSplatInstancing& instancing,
    const bool                          isWire,
    const PXR_NS::SdfPath&              id)
{
    PXR_NS::HdVP2DrawItem::RenderItemData& itemData = stateToCommit._renderItemData;

    unsigned int& oldInstanceCount = itemData._instanceCount;
    if (instancing._instanced) {
        const std::shared_ptr<MMatrixArray>& transforms = stateToCommit._instanceTransforms;
        const unsigned int newInstanceCount = transforms ? transforms->length() : 0;
        bool               arrayReset = false;

        //! Replace the whole instance array. Returns true on success.
        const auto setInstanceArray = [&]() {
            arrayReset
                = static_cast<bool>(drawScene.setInstanceTransformArray(renderItem, *transforms));
            if (!arrayReset) {
                TF_WARN(
                    "Gaussian splat %s: could not set %u instance transforms.",
                    id.GetText(),
                    newInstanceCount);
            }
            return arrayReset;
        };

        if (transforms) {
            if (!itemData._usingInstancedDraw) {
                if (newInstanceCount >= 1) {
                    // The instance matrices include the world matrix of the prim.
                    const MMatrix identity;
                    renderItem.setMatrix(&identity);
                    if (setInstanceArray()) {
                        itemData._usingInstancedDraw = true;
                    }
                }
            } else if (newInstanceCount == 0) {
                // The item is disabled. Drop the stale instances.
                drawScene.removeAllInstances(renderItem);
                drawScene.removeExtraInstanceData(renderItem, PXR_NS::MayaUsdRPrim::kSolidColorStr);
            } else if (oldInstanceCount == newInstanceCount) {
                for (unsigned int i = 0; i < newInstanceCount; ++i) {
                    // VP2 defines instance ID of the first instance to be 1.
                    drawScene.updateInstanceTransform(renderItem, i + 1, (*transforms)[i]);
                }
            } else {
                setInstanceArray();
            }
            oldInstanceCount = newInstanceCount;

#ifdef MAYA_MRENDERITEM_UFE_IDENTIFIER_SUPPORT
            drawScene.setUfeIdentifiers(renderItem, stateToCommit._ufeIdentifiers);
#endif
#ifdef MAYA_NEW_POINT_SNAPPING_SUPPORT
            PXR_NS::MayaUsdCustomData::Get(renderItem) = *instancing._mayaToUsdIds;
#endif
        }

        // One tint per instance replaces the shader parameter for every instance, so the data is
        // sent whenever the instance array was reset or the tints changed.
        const std::shared_ptr<MFloatArray>& instanceTints = instancing._tints;
        if (!isWire && itemData._usingInstancedDraw && instanceTints && instanceTints->length() > 0
            && instanceTints->length() == oldInstanceCount * 4
            && (arrayReset || instancing._tintsChanged)) {
            const MStatus status = drawScene.setExtraInstanceData(
                renderItem, PXR_NS::MayaUsdRPrim::kSolidColorStr, *instanceTints);
            if (!status) {
                TF_WARN("Gaussian splat %s: could not set the instance tints.", id.GetText());
            }
        }
    } else if (stateToCommit._worldMatrix != nullptr) {
        renderItem.setMatrix(stateToCommit._worldMatrix);
    }
}

} // namespace MAYAUSD_NS_DEF

#endif // PXR_VERSION >= 2605
