//
// Copyright 2017 Animal Logic
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

#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"

#include <mayaUsd/nodes/proxyShapePlugin.h>

#include <maya/MProfiler.h>
namespace {
const int _proxyShapeMetadataProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "AL_usdmaya_ProxyShape_selection",
    "AL_usdmaya_ProxyShape_selection"
#else
    "AL_usdmaya_ProxyShape_selection"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::processChangedMetaData(
    const SdfPathVector& resyncedPaths,
    const SdfPathVector& changedOnlyPaths)
{
    MProfilingScope profilerScope(
        _proxyShapeMetadataProfilerCategory, MProfiler::kColorE_L3, "Process changed metadata");

    TF_DEBUG(ALUSDMAYA_EVENTS)
        .Msg(
            "ProxyShape::processChangedMetaData - processing changes %lu %lu\n",
            resyncedPaths.size(),
            changedOnlyPaths.size());

    bool excludedPrimsModified = false;
    {
        for (const SdfPath& resyncedPath : resyncedPaths) {
            const UsdPrim syncPrimRoot = m_stage->GetPrimAtPath(resyncedPath);
            if (!syncPrimRoot) {
                // TODO : Ensure elements have been removed from selectabilityDB, excludeGeom, and
                // lock prims
                continue;
            }

            // sort the excluded tagged geom to help searching
            std::sort(m_excludedTaggedGeometry.begin(), m_excludedTaggedGeometry.end());

            // keep track of where the last item in the original set of prims is.
            // As we traverse, lookups will be performed on the first elements in the array, so that
            // will remain sorted. Items appended to the end of the array will not be sorted.
            // In order to do this without using a 2nd vector, I'm using the lastTaggedPrim to
            // denote the boundary between the sorted and unsorted prims
            size_t lastTaggedPrim = m_excludedTaggedGeometry.size();

            // from the resync prim, traverse downwards through the child prims
            for (fileio::TransformIterator it(syncPrimRoot, parentTransform(), true); !it.done();
                 it.next()) {
                const auto& prim = it.prim();
                const auto& path = prim.GetPath();

                // first check to see if the excluded geom has changed
                {
                    bool excludeGeo = false;
                    if (prim.GetMetadata(Metadata::excludeFromProxyShape, &excludeGeo)
                        && excludeGeo) {
                        const auto last = m_excludedTaggedGeometry.begin() + lastTaggedPrim;
                        const auto geoIt
                            = std::lower_bound(m_excludedTaggedGeometry.begin(), last, path);
                        if (geoIt != last && *geoIt == path) {
                            // we already have an entry for this prim
                        } else {
                            // add to back of list
                            excludedPrimsModified = true;
                            m_excludedTaggedGeometry.emplace_back(path);
                        }
                    } else {
                        // if we aren't excluding the geom, but have an existing entry, remove it.
                        const auto last = m_excludedTaggedGeometry.begin() + lastTaggedPrim;
                        const auto geoIt
                            = std::lower_bound(m_excludedTaggedGeometry.begin(), last, path);
                        if (geoIt != last && *geoIt == path) {
                            excludedPrimsModified = true;
                            m_excludedTaggedGeometry.erase(geoIt);
                            --lastTaggedPrim;
                        }
                    }
                }
            }
        }
    }

    // reconstruct the lock prims
    {
        constructLockPrims();
    }

    if (excludedPrimsModified) {
        if (MayaUsdProxyShapePlugin::useVP2_NativeUSD_Rendering()) {
            _IncreaseExcludePrimPathsVersion();
        } else {
            constructGLImagingEngine();
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructExcludedPrims()
{
    MProfilingScope profilerScope(
        _proxyShapeMetadataProfilerCategory, MProfiler::kColorE_L3, "Construct excluded prims");

    auto excludedPaths = getExcludePrimPaths();
    if (m_excludedGeometry != excludedPaths) {
        std::swap(m_excludedGeometry, excludedPaths);
        _IncreaseExcludePrimPathsVersion();
        constructGLImagingEngine();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructLockPrims()
{
    MProfilingScope profilerScope(
        _proxyShapeMetadataProfilerCategory, MProfiler::kColorE_L3, "Construct lock prims");

    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::constructLockPrims\n");

    LockPrimCache cache;
    // iterate over the
    for (const auto& it : m_requiredPaths) {
        Scope* transformScope = it.second.getTransformNode();
        if (!transformScope) {
            continue;
        }

        const UsdPrim& prim = transformScope->transform()->prim();
        if (prim) {
            bool isLocked = isPrimLocked(prim, cache);

            MObject lockObject = transformScope->thisMObject();

            MPlug t(lockObject, m_transformTranslate);
            MPlug r(lockObject, m_transformRotate);
            MPlug s(lockObject, m_transformScale);

            t.setLocked(isLocked);
            r.setLocked(isLocked);
            s.setLocked(isLocked);

            if (isLocked) {
                MFnDependencyNode fn(lockObject);
                if (dynamic_cast<Transform*>(fn.userNode())) {
                    MPlug plug(lockObject, Transform::pushToPrim());
                    if (plug.asBool())
                        plug.setBool(false);
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::primHasExcludedParent(UsdPrim prim)
{
    MProfilingScope profilerScope(
        _proxyShapeMetadataProfilerCategory,
        MProfiler::kColorE_L3,
        "Check excluded parent for prim");

    if (prim.IsValid()) {
        SdfPath primPath = prim.GetPrimPath();
        TF_FOR_ALL(excludedPath, m_excludedTaggedGeometry)
        {
            if (primPath.HasPrefix(*excludedPath)) {
                TF_DEBUG(ALUSDMAYA_EVALUATION)
                    .Msg("ProxyShape::primHasExcludedParent %s=true\n", primPath.GetText());
                return true;
            }
        }
    }

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::findPrimsWithMetaData()
{
    MProfilingScope profilerScope(
        _proxyShapeMetadataProfilerCategory, MProfiler::kColorE_L3, "Find prims with metadata");

    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::findPrimsWithMetaData\n");
    if (!m_stage)
        return;

    if (isLockPrimFeatureActive()) {
        for (fileio::TransformIterator it(m_stage, parentTransform(), true); !it.done();
             it.next()) {
            const auto& prim = it.prim();
            bool excludeGeo = false;
            if (prim.GetMetadata(Metadata::excludeFromProxyShape, &excludeGeo)) {
                if (excludeGeo) {
                    m_excludedTaggedGeometry.push_back(prim.GetPrimPath());
                }
            }
        }

        constructLockPrims();
        constructExcludedPrims();
    }
}

//----------------------------------------------------------------------------------------------------------------------
SdfPathVector ProxyShape::getExcludePrimPaths() const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getExcludePrimPaths\n");

    SdfPathVector paths = m_excludedTaggedGeometry;
    SdfPathVector temp = getPrimPathsFromCommaJoinedString(excludePrimPathsPlug().asString());
    paths.insert(paths.end(), temp.begin(), temp.end());
    temp = getPrimPathsFromCommaJoinedString(excludedTranslatedGeometryPlug().asString());
    paths.insert(paths.end(), temp.begin(), temp.end());

    const auto& translatedGeo = m_context->excludedGeometry();

    // combine the excluded paths
    paths.insert(paths.end(), m_excludedTaggedGeometry.begin(), m_excludedTaggedGeometry.end());
    paths.insert(paths.end(), m_excludedGeometry.begin(), m_excludedGeometry.end());
    for (auto& it : translatedGeo) {
        paths.push_back(it.second);
    }

    return paths;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
