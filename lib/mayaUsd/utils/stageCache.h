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
#ifndef PXRUSDMAYA_STAGECACHE_H
#define PXRUSDMAYA_STAGECACHE_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageCache.h>

#include <array>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaStageCache
{
public:
    /// The shared mode of stage kept in a particular cache.
    ///
    /// Shared stages allow staging the same root layer multiple times in Maya
    /// with the same session layer.
    ///
    /// Unshared stages ensure they do not share their session layer.
    enum class ShareMode
    {
        Shared,
        Unshared
    };

    /// Container of caches.
    using Caches = std::array<UsdStageCache, 4>;

    /// Return all the stage caches.
    MAYAUSD_CORE_PUBLIC
    static Caches& GetAllCaches();

    /// Return the singleton stage cache for use by all USD clients within Maya.
    /// Four stage caches are maintained. They are divided based on two criteria:
    ///
    ///    stages that have been opened with UsdStage::InitialLoadSet::LoadAll
    ///                                  vs
    ///    stages that have been opened with UsdStage::InitialLoadSet::LoadNone.
    ///
    ///    stages that are shared
    ///            vs
    ///    stages that are not-shared
    MAYAUSD_CORE_PUBLIC
    static UsdStageCache& Get(UsdStage::InitialLoadSet, ShareMode);

    /// Clear the cache
    MAYAUSD_CORE_PUBLIC
    static void Clear();

    /// Erase all stages from the stage caches whose root layer path is
    /// \p layerPath.
    ///
    /// The number of stages erased from the caches is returned.
    MAYAUSD_CORE_PUBLIC
    static size_t EraseAllStagesWithRootLayerPath(const std::string& layerPath);

    /// Gets (or creates) a shared session layer tied with the given variant
    /// selections and draw mode on the given root path.
    /// The stage is cached for the lifetime of the current Maya scene.
    MAYAUSD_CORE_PUBLIC
    static SdfLayerRefPtr GetSharedSessionLayer(
        const SdfPath&                            rootPath,
        const std::map<std::string, std::string>& variantSelections,
        const TfToken&                            drawMode);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
