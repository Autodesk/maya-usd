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
void ProxyShape::removeMetaData(const SdfPathVector& removedPaths)
{
    MProfilingScope profilerScope(
        _proxyShapeMetadataProfilerCategory, MProfiler::kColorE_L3, "Remove metadata");

    TF_DEBUG(ALUSDMAYA_EVENTS).Msg("ProxyShape::removeMetaData");

    m_selectabilityDB.removePathsAsUnselectable(removedPaths);
    m_lockManager.removeEntries(removedPaths);
}

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

    // if this is just a data change (rather than a variant switch), check for any meta data changes
    if (m_variantSwitchedPrims.empty()) {
        // figure out whether selectability has changed.
        for (const SdfPath& path : changedOnlyPaths) {
            UsdPrim changedPrim = m_stage->GetPrimAtPath(path);
            if (!changedPrim) {
                continue;
            }

            TfToken selectabilityPropertyToken;
            if (changedPrim.GetMetadata<TfToken>(
                    Metadata::selectability, &selectabilityPropertyToken)) {
                bool hasPath = m_selectabilityDB.containsPath(path);
                // Check if this prim is unselectable
                if (selectabilityPropertyToken == Metadata::unselectable) {
                    if (!hasPath) {
                        m_selectabilityDB.addPathAsUnselectable(path);
                    }
                }
                if (hasPath) {
                    m_selectabilityDB.removePathAsUnselectable(path);
                }
            }

            // build up new lock-prim list
            TfToken lockPropertyToken;
            if (changedPrim.GetMetadata<TfToken>(Metadata::locked, &lockPropertyToken)) {
                if (lockPropertyToken == Metadata::lockTransform) {
                    m_lockManager.setLocked(path);
                } else if (lockPropertyToken == Metadata::lockUnlocked) {
                    m_lockManager.setUnlocked(path);
                } else {
                    m_lockManager.setInherited(path);
                }
            } else {
                m_lockManager.setInherited(path);
            }
        }
        m_lockManager.sort();
    } else {
        auto& unselectablePaths = m_selectabilityDB.m_unselectablePaths;

        // figure out whether selectability has changed.
        for (const SdfPath& path : resyncedPaths) {
            UsdPrim syncPrimRoot = m_stage->GetPrimAtPath(path);
            if (!syncPrimRoot) {
                // TODO : Ensure elements have been removed from selectabilityDB, excludeGeom, and
                // lock prims
                continue;
            }

            // determine range of selectable prims we need to replace in the selectability DB
            auto lb = std::lower_bound(unselectablePaths.begin(), unselectablePaths.end(), path);
            auto ub = lb;
            if (lb != unselectablePaths.end()) {
                while (ub != unselectablePaths.end() && ub->HasPrefix(*lb)) {
                    ++ub;
                }
            }

            m_lockManager.removeFromRootPath(path);

            // sort the excluded tagged geom to help searching
            std::sort(m_excludedTaggedGeometry.begin(), m_excludedTaggedGeometry.end());

            // fill these lists as we traverse
            SdfPathVector newUnselectables;
            SdfPathVector newExcludeGeom;

            // keep track of where the last item in the original set of prims is.
            // As we traverse, lookups will be performed on the first elements in the array, so that
            // will remain sorted. Items appended to the end of the array will not be sorted.
            // In order to do this without using a 2nd vector, I'm using the lastTaggedPrim to
            // denote the boundary between the sorted and unsorted prims
            size_t lastTaggedPrim = m_excludedTaggedGeometry.size();

            // from the resync prim, traverse downwards through the child prims
            for (fileio::TransformIterator it(syncPrimRoot, parentTransform(), true); !it.done();
                 it.next()) {
                auto prim = it.prim();
                auto path = prim.GetPath();

                // first check to see if the excluded geom has changed
                {
                    bool excludeGeo = false;
                    if (prim.GetMetadata(Metadata::excludeFromProxyShape, &excludeGeo)
                        && excludeGeo) {
                        auto last = m_excludedTaggedGeometry.begin() + lastTaggedPrim;
                        auto it = std::lower_bound(m_excludedTaggedGeometry.begin(), last, path);
                        if (it != last && *it == path) {
                            // we already have an entry for this prim
                        } else {
                            // add to back of list
                            m_excludedTaggedGeometry.emplace_back(path);
                        }
                    } else {
                        // if we aren't excluding the geom, but have an existing entry, remove it.
                        auto last = m_excludedTaggedGeometry.begin() + lastTaggedPrim;
                        auto it = std::lower_bound(m_excludedTaggedGeometry.begin(), last, path);
                        if (it != last && *it == path) {
                            m_excludedTaggedGeometry.erase(it);
                            --lastTaggedPrim;
                        }
                    }

                    // If prim has exclusion tag or is a descendent of a prim with it, create as
                    // Maya geo
                    if (excludeGeo || primHasExcludedParent(prim)) {
                        VtValue schemaName(fileio::ALExcludedPrimSchema.GetString());
                        prim.SetCustomDataByKey(fileio::ALSchemaType, schemaName);
                    }
                }

                // build up new unselectable list
                TfToken selectabilityPropertyToken;
                if (prim.GetMetadata<TfToken>(
                        Metadata::selectability, &selectabilityPropertyToken)) {
                    if (selectabilityPropertyToken == Metadata::unselectable) {
                        newUnselectables.emplace_back(path);
                    }
                }

                // build up new lock-prim list
                TfToken lockPropertyToken;
                if (prim.GetMetadata<TfToken>(Metadata::locked, &lockPropertyToken)) {
                    if (lockPropertyToken == Metadata::lockTransform) {
                        m_lockManager.addLocked(path);
                    } else if (lockPropertyToken == Metadata::lockUnlocked) {
                        m_lockManager.addUnlocked(path);
                    }
                }
            }

            m_lockManager.sort();

            // sort and merge into previous selectable database
            std::sort(newUnselectables.begin(), newUnselectables.end());

            if (lb == unselectablePaths.end()) {
                if (!newUnselectables.empty()) {
                    unselectablePaths.insert(
                        unselectablePaths.end(), newUnselectables.begin(), newUnselectables.end());
                }
            } else {
                // TODO: could probably overwrite some of old paths here, and then insert the
                // remaining new elements.
                lb = unselectablePaths.erase(lb, ub);
                if (!newUnselectables.empty()) {
                    unselectablePaths.insert(lb, newUnselectables.begin(), newUnselectables.end());
                }
            }
        }
    }

    // reconstruct the lock prims
    {
        constructLockPrims();
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

    // iterate over the
    for (auto it : m_requiredPaths) {
        Scope* s = it.second.getTransformNode();
        if (!s) {
            continue;
        }

        const UsdPrim& prim = s->transform()->prim();
        if (prim) {
            bool is_locked = m_lockManager.isLocked(prim.GetPath());

            MObject lockObject = s->thisMObject();

            MPlug t(lockObject, m_transformTranslate);
            MPlug r(lockObject, m_transformRotate);
            MPlug s(lockObject, m_transformScale);

            t.setLocked(is_locked);
            r.setLocked(is_locked);
            s.setLocked(is_locked);

            if (is_locked) {
                MFnDependencyNode fn(lockObject);
                Transform*        transformNode = dynamic_cast<Transform*>(fn.userNode());
                if (transformNode) {
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
        SdfPathVector newUnselectables;
        for (fileio::TransformIterator it(m_stage, parentTransform(), true); !it.done();
             it.next()) {
            auto prim = it.prim();
            bool excludeGeo = false;
            if (prim.GetMetadata(Metadata::excludeFromProxyShape, &excludeGeo)) {
                if (excludeGeo) {
                    m_excludedTaggedGeometry.push_back(prim.GetPrimPath());
                }
            }

            // If prim has exclusion tag or is a descendent of a prim with it, create as Maya geo
            if (excludeGeo || primHasExcludedParent(prim)) {
                VtValue schemaName(fileio::ALExcludedPrimSchema.GetString());
                prim.SetCustomDataByKey(fileio::ALSchemaType, schemaName);
            }

            TfToken selectabilityPropertyToken;
            if (prim.GetMetadata<TfToken>(Metadata::selectability, &selectabilityPropertyToken)) {
                // Check if this prim is unselectable
                if (selectabilityPropertyToken == Metadata::unselectable) {
                    newUnselectables.push_back(prim.GetPath());
                }
            }

            // build up new lock-prim list
            TfToken lockPropertyToken;
            if (prim.GetMetadata<TfToken>(Metadata::locked, &lockPropertyToken)) {
                if (lockPropertyToken == Metadata::lockTransform) {
                    m_lockManager.setLocked(prim.GetPath());
                } else if (lockPropertyToken == Metadata::lockUnlocked) {
                    m_lockManager.setUnlocked(prim.GetPath());
                } else if (lockPropertyToken == Metadata::lockInherited) {
                    m_lockManager.setInherited(prim.GetPath());
                }
            } else {
                m_lockManager.setInherited(prim.GetPath());
            }
        }

        constructLockPrims();
        constructExcludedPrims();
        m_selectabilityDB.setPathsAsUnselectable(newUnselectables);
    }
}

//----------------------------------------------------------------------------------------------------------------------
SdfPathVector ProxyShape::getExcludePrimPaths() const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getExcludePrimPaths\n");

    SdfPathVector paths = getPrimPathsFromCommaJoinedString(excludePrimPathsPlug().asString());
    SdfPathVector temp
        = getPrimPathsFromCommaJoinedString(excludedTranslatedGeometryPlug().asString());
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
