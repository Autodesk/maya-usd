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
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"

#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <maya/MProfiler.h>
namespace {
const int _primFilterProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "PrimFIlter",
    "PrimFIlter"
#else
    "PrimFIlter"
#endif
);
} // namespace

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
PrimFilter::PrimFilter(
    const SdfPathVector&        previousPrims,
    const std::vector<UsdPrim>& newPrimSet,
    PrimFilterInterface*        proxy,
    bool                        forceImport)
    : m_newPrimSet(newPrimSet)
    , m_transformsToCreate()
    , m_updatablePrimSet()
    , m_removedPrimSet()
{
    MProfilingScope profilerScope(
        _primFilterProfilerCategory, MProfiler::kColorE_L3, "Initialise prim filter");

    // copy over original prims
    m_removedPrimSet.assign(previousPrims.begin(), previousPrims.end());
    std::sort(
        m_removedPrimSet.begin(), m_removedPrimSet.end(), [](const SdfPath& a, const SdfPath& b) {
            return b < a;
        });

    for (auto it = m_newPrimSet.begin(); it != m_newPrimSet.end();) {
        UsdPrim prim = *it;
        auto    lastIt = it;
        ++it;

        SdfPath path = prim.GetPath();

        // check previous prim type (if it exists at all?)
        std::string existingTranslatorId = proxy->getTranslatorIdForPath(path);

        std::string newTranslatorId = proxy->generateTranslatorId(prim);

        // inactive prims should be removed
        if (!prim.IsActive()) {
            it = m_newPrimSet.erase(lastIt);
            continue;
        }

        bool supportsUpdate = false;
        bool requiresParent = false;
        bool importableByDefault = false;
        proxy->getTranslatorInfo(
            newTranslatorId, supportsUpdate, requiresParent, importableByDefault);

        if (importableByDefault || forceImport) {
            // if the type remains the same, and the type supports update
            if (existingTranslatorId == newTranslatorId) {
                // locate the path and delete from the removed set (we do not want to delete this
                // prim! Note that m_removedPrimSet is reverse sorted
                auto iter = std::lower_bound(
                    m_removedPrimSet.begin(),
                    m_removedPrimSet.end(),
                    path,
                    [](const SdfPath& a, const SdfPath& b) { return b < a; });
                if (iter != removedPrimSet().end() && *iter == path) {
                    if (supportsUpdate) {
                        m_removedPrimSet.erase(iter);
                        if (proxy->isPrimDirty(prim)) {
                            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                                .Msg(
                                    "PrimFilter::PrimFilter %s prim will be updated.\n",
                                    path.GetText());
                            m_updatablePrimSet.push_back(prim);
                        } else {
                            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                                .Msg(
                                    "PrimFilter::PrimFilter %s prim remains unchanged.\n",
                                    path.GetText());
                        }
                        // supporting update means it's not a new prim,
                        // otherwise we still want the prim to be re-created.
                        it = m_newPrimSet.erase(lastIt);

                        // skip creating transforms in this case.
                        requiresParent = false;
                    } else {
                        if (proxy->isPrimDirty(prim)) {
                            // prim has been added in "remove prim set", nothing to do here
                            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                                .Msg(
                                    "PrimFilter::PrimFilter %s prim will be removed and "
                                    "recreated.\n",
                                    path.GetText());
                        } else {
                            // prim is clean, no need to remove nor recreate
                            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                                .Msg(
                                    "PrimFilter::PrimFilter %s prim remains unchanged.\n",
                                    path.GetText());

                            m_removedPrimSet.erase(iter);
                            it = m_newPrimSet.erase(lastIt);
                            // skip creating transforms in this case.
                            requiresParent = false;
                        }
                    }
                }
            }
            // if we need a transform, make a note of it now
            if (requiresParent) {
                m_transformsToCreate.push_back(prim);
            }
        } else {
            it = m_newPrimSet.erase(lastIt);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace proxy
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
