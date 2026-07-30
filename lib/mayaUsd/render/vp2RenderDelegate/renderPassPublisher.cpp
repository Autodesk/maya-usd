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
#include "renderPassPublisher.h"

#include "debugCodes.h"

#include <pxr/base/tf/staticTokens.h>
#include <pxr/imaging/hd/collectionSchema.h>
#include <pxr/imaging/hd/collectionsSchema.h>
#include <pxr/imaging/hd/filteringSceneIndex.h>
#include <pxr/imaging/hd/mergingSceneIndex.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usd/collectionAPI.h>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderVisibility)
    (prune)
);
// clang-format on

namespace {

// Walks down from the terminal scene index to the first HdMergingSceneIndex,
// which under emulation is the one holding the legacy prim chain as an input.
HdMergingSceneIndexRefPtr _FindMergingSceneIndex(const HdSceneIndexBaseRefPtr& sceneIndex)
{
    if (!sceneIndex) {
        return nullptr;
    }
    if (const auto merging = TfDynamic_cast<HdMergingSceneIndexRefPtr>(sceneIndex)) {
        return merging;
    }
    if (const auto filtering = TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(sceneIndex)) {
        for (const HdSceneIndexBaseRefPtr& input : filtering->GetInputScenes()) {
            if (const auto found = _FindMergingSceneIndex(input)) {
                return found;
            }
        }
    }
    return nullptr;
}

// Collection expressions are authored against raw USD paths, but
// UsdImagingDelegate prefixes everything it emits with its delegate ID (see
// UsdImagingDelegate::ConvertCachePathToIndexPath), so an unmodified expression
// would match nothing in the emulated scene.
SdfPathExpression _RebaseOntoDelegate(const SdfPathExpression& expr, const SdfPath& delegateId)
{
    if (expr.IsEmpty() || delegateId.IsEmpty() || delegateId == SdfPath::AbsoluteRootPath()) {
        return expr;
    }
    return expr.ReplacePrefix(SdfPath::AbsoluteRootPath(), delegateId);
}

SdfPathExpression
_ReadCollection(const UsdPrim& passPrim, const TfToken& name, const SdfPath& delegateId)
{
    // Only the membershipExpression form is supported: it is the only form
    // HdCollectionSchema carries, so it is also all hdPrman's filter consumes.
    // UsdCollectionAPI::Get succeeds whether or not the API is actually applied,
    // so test the attribute rather than the schema object.
    const UsdCollectionAPI collection = UsdCollectionAPI::Get(passPrim, name);
    if (!collection.GetMembershipExpressionAttr().HasAuthoredValue()) {
        TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
            .Msg(
                "  collection '%s': no authored membershipExpression on <%s>\n",
                name.GetText(),
                passPrim.GetPath().GetText());
        return SdfPathExpression();
    }

    const SdfPathExpression authored = collection.ResolveCompleteMembershipExpression();
    const SdfPathExpression rebased = _RebaseOntoDelegate(authored, delegateId);
    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "  collection '%s': authored '%s' -> rebased '%s'\n",
            name.GetText(),
            authored.GetText().c_str(),
            rebased.GetText().c_str());
    return rebased;
}

HdContainerDataSourceHandle
_BuildPassDataSource(const SdfPathExpression& pruneExpr, const SdfPathExpression& renderVisExpr)
{
    TfTokenVector                     names;
    std::vector<HdDataSourceBaseHandle> values;

    auto addCollection = [&](const TfToken& name, const SdfPathExpression& expr) {
        if (expr.IsEmpty()) {
            return;
        }
        names.push_back(name);
        values.push_back(HdCollectionSchema::Builder()
                             .SetMembershipExpression(
                                 HdRetainedTypedSampledDataSource<SdfPathExpression>::New(expr))
                             .Build());
    };

    addCollection(_tokens->prune, pruneExpr);
    addCollection(_tokens->renderVisibility, renderVisExpr);

    if (names.empty()) {
        return nullptr;
    }

    return HdRetainedContainerDataSource::New(
        HdCollectionsSchema::GetSchemaToken(),
        HdCollectionsSchema::BuildRetained(names.size(), names.data(), values.data()));
}

} // namespace

MayaUsdRenderPassPublisher::MayaUsdRenderPassPublisher(const HdSceneIndexBaseRefPtr& inputScene)
{
    _retained = HdRetainedSceneIndex::New();

    HdMergingSceneIndexRefPtr merging = HdMergingSceneIndex::New();
    merging->AddInputScene(inputScene, SdfPath::AbsoluteRootPath());
    merging->AddInputScene(_retained, SdfPath::AbsoluteRootPath());

    _sceneGlobals = HdsiSceneGlobalsSceneIndex::New(merging);
    _filter = MayaUsdRenderPassSceneIndex::New(_sceneGlobals);
    _terminal = _filter;
}

void MayaUsdRenderPassPublisher::Publish(
    const UsdStageRefPtr& stage,
    const SdfPath&        passPath,
    const SdfPath&        delegateId)
{
    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg(
            "Publish: pass <%s>, stage %s, delegateId <%s>\n",
            passPath.GetText(),
            stage ? "valid" : "NULL",
            delegateId.GetText());

    HdContainerDataSourceHandle passDataSource;
    if (stage && !passPath.IsEmpty()) {
        const UsdPrim passPrim = stage->GetPrimAtPath(passPath);
        if (passPrim) {
            passDataSource = _BuildPassDataSource(
                _ReadCollection(passPrim, _tokens->prune, delegateId),
                _ReadCollection(passPrim, _tokens->renderVisibility, delegateId));
        } else {
            TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
                .Msg("  no prim at <%s> on the stage\n", passPath.GetText());
        }
    }

    // Place the synthetic prim under the delegate prefix so it cannot collide
    // with another proxy shape sharing this render index.
    SdfPath publishPath;
    if (passDataSource) {
        publishPath = delegateId.IsEmpty() || delegateId == SdfPath::AbsoluteRootPath()
            ? passPath
            : passPath.ReplacePrefix(SdfPath::AbsoluteRootPath(), delegateId);
    }

    if (publishPath.IsEmpty()) {
        // Deactivate before removing so the filter recomputes against a scene
        // that still contains the prim it is dropping.
        _sceneGlobals->SetActiveRenderPassPrimPath(SdfPath::EmptyPath());
        if (!_publishedPassPath.IsEmpty()) {
            _retained->RemovePrims({ { _publishedPassPath } });
            _publishedPassPath = SdfPath::EmptyPath();
        }
        return;
    }

    if (!_publishedPassPath.IsEmpty() && _publishedPassPath != publishPath) {
        _retained->RemovePrims({ { _publishedPassPath } });
    }

    // Re-adding an existing path replaces it and emits PrimsAdded, which is the
    // signal the filter uses to recompute its state.
    _retained->AddPrims({ { publishPath, HdPrimTypeTokens->renderPass, passDataSource } });
    _publishedPassPath = publishPath;

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg("  published synthetic pass prim at <%s>, activating\n", publishPath.GetText());

    _sceneGlobals->SetActiveRenderPassPrimPath(publishPath);
}

/* static */
std::unique_ptr<MayaUsdRenderPassPublisher>
MayaUsdRenderPassPublisher::Attach(HdRenderIndex& renderIndex)
{
    if (!HdRenderIndex::IsSceneIndexEmulationEnabled()) {
        TF_WARN("Scene index emulation is disabled; USD render pass filtering is unavailable.");
        return nullptr;
    }

    const HdMergingSceneIndexRefPtr merging
        = _FindMergingSceneIndex(renderIndex.GetTerminalSceneIndex());
    if (!merging) {
        TF_WARN("No merging scene index found; USD render pass filtering is unavailable.");
        return nullptr;
    }

    const std::vector<HdSceneIndexBaseRefPtr> inputs = merging->GetInputScenes();
    if (inputs.size() != 1) {
        TF_WARN(
            "Expected exactly one input on the render index's merging scene index, found %zu; "
            "USD render pass filtering is unavailable.",
            inputs.size());
        return nullptr;
    }

    const HdSceneIndexBaseRefPtr emulation = inputs.front();
    auto publisher = std::make_unique<MayaUsdRenderPassPublisher>(emulation);

    // Remove before re-inserting: adding the filter alone would make it a
    // sibling of the emulation input rather than wrap it.
    renderIndex.RemoveSceneIndex(emulation);
    renderIndex.InsertSceneIndex(publisher->Terminal(), SdfPath::AbsoluteRootPath());

    TF_DEBUG(HDVP2_DEBUG_RENDER_PASS)
        .Msg("Interposed render pass filter over '%s'\n", emulation->GetDisplayName().c_str());

    return publisher;
}

PXR_NAMESPACE_CLOSE_SCOPE
