//
// Copyright 2016 Pixar
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
#include "stageCache.h"

#include <mayaUsd/listeners/notice.h>

#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/primSpec.h>
#include <pxr/usd/sdf/relationshipSpec.h>
#include <pxr/usd/usd/stageCache.h>
#include <pxr/usd/usdGeom/tokens.h>

#include <maya/MFileIO.h>
#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>

#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static std::map<std::string, SdfLayerRefPtr> _sharedSessionLayers;
static std::mutex                            _sharedSessionLayersMutex;

struct _OnSceneResetListener : public TfWeakBase
{
    _OnSceneResetListener()
    {
        TfWeakPtr<_OnSceneResetListener> me(this);
        TfNotice::Register(me, &_OnSceneResetListener::OnSceneReset);
    }

    void OnSceneReset(const UsdMayaSceneResetNotice& notice)
    {
        UsdMayaStageCache::Clear();

        std::lock_guard<std::mutex> lock(_sharedSessionLayersMutex);
        _sharedSessionLayers.clear();
    }
};

void clearMayaAttributeEditor()
{
    // When a stage is deleted, the attribute editor could still refer to prims
    // that were on that stage. If the attribute editor is collapsed, then it
    // won't refresh itself and could later on try to access the prim.
    //
    // This happens when it receives a UFE notification that it thinks is about
    // the prim it is showing. This only happens if one re-stage the same file,
    // as the UFE notification will contain the same stage name and the same
    // prim path.
    //
    // To avoid crashes, we refresh the attribute editor templates when the
    // stages get cleared.
    MGlobal::executeCommand("refreshEditorTemplates");
}

} // anonymous namespace

/*static */
UsdMayaStageCache::Caches& UsdMayaStageCache::GetAllCaches()
{
    static Caches                caches;
    static _OnSceneResetListener onSceneResetListener;
    return caches;
}

/* static */
UsdStageCache& UsdMayaStageCache::Get(UsdStage::InitialLoadSet loadSet, ShareMode shared)
{
    // The different caches are separated by:
    //
    //    - load all payload / load nothing
    //    - shared stages / unshared stages
    //
    // The load all / load nothing correspond to the UsdStage::InitialLoadSet::LoadAll /
    // UsdStage::InitialLoadSet::LoadNode mode of UsdStage::Open

    // Note: each criteria uses increasing power of two to select among the array
    //       of cache. If you add new criteria, the new indexes will be 4, 8, 16...
    const int loadSetIndex = (loadSet == UsdStage::InitialLoadSet::LoadAll) ? 0 : 1;
    const int sharedIndex = (shared == ShareMode::Shared) ? 0 : 2;

    auto& caches = GetAllCaches();
    return caches[loadSetIndex + sharedIndex];
}

/* static */
void UsdMayaStageCache::Clear()
{
    clearMayaAttributeEditor();
    for (auto& cache : GetAllCaches())
        cache.Clear();
}

/* static */
size_t UsdMayaStageCache::EraseAllStagesWithRootLayerPath(const std::string& layerPath)
{
    size_t erasedStages = 0u;

    const SdfLayerHandle rootLayer = SdfLayer::Find(layerPath);
    if (!rootLayer) {
        return erasedStages;
    }

    clearMayaAttributeEditor();

    for (auto& cache : GetAllCaches())
        erasedStages += cache.EraseAll(rootLayer);

    return erasedStages;
}

SdfLayerRefPtr UsdMayaStageCache::GetSharedSessionLayer(
    const SdfPath&                            rootPath,
    const std::map<std::string, std::string>& variantSelections,
    const TfToken&                            drawMode)
{
    // Example key: "/Root/Path:modelingVariant=round|shadingVariant=red|:cards"
    std::ostringstream key;
    key << rootPath;
    key << ":";
    for (const auto& pair : variantSelections) {
        key << pair.first << "=" << pair.second << "|";
    }
    key << ":";
    key << drawMode;

    std::string                 keyString = key.str();
    std::lock_guard<std::mutex> lock(_sharedSessionLayersMutex);
    auto                        iter = _sharedSessionLayers.find(keyString);
    if (iter == _sharedSessionLayers.end()) {
        SdfLayerRefPtr newLayer = SdfLayer::CreateAnonymous();

        SdfPrimSpecHandle over = SdfCreatePrimInLayer(newLayer, rootPath);
        for (const auto& pair : variantSelections) {
            const std::string& variantSet = pair.first;
            const std::string& variantSelection = pair.second;
            over->GetVariantSelections()[variantSet] = variantSelection;
        }

        if (!drawMode.IsEmpty()) {
            SdfAttributeSpecHandle drawModeAttr = SdfAttributeSpec::New(
                over,
                UsdGeomTokens->modelDrawMode,
                SdfValueTypeNames->Token,
                SdfVariabilityUniform);
            drawModeAttr->SetDefaultValue(VtValue(drawMode));
            SdfAttributeSpecHandle applyDrawModeAttr = SdfAttributeSpec::New(
                over,
                UsdGeomTokens->modelApplyDrawMode,
                SdfValueTypeNames->Bool,
                SdfVariabilityUniform);
            applyDrawModeAttr->SetDefaultValue(VtValue(true));
        }

        _sharedSessionLayers[keyString] = newLayer;
        return newLayer;
    } else {
        return iter->second;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
