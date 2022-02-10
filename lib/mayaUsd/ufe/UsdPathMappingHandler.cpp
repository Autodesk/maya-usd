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
#include "UsdPathMappingHandler.h"

#include "UsdSceneItem.h"

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>

#include <ufe/scene.h>
#include <ufe/trie.imp.h>

#include <algorithm>

// Needed because of TF_CODING_ERROR
PXR_NAMESPACE_USING_DIRECTIVE

namespace {
Ufe::Trie<Ufe::Path> fromHostCache;

class UfeObserver : public Ufe::Observer
{
public:
    UfeObserver()
        : Ufe::Observer()
    {
    }

    void operator()(const Ufe::Notification& notification) override
    {
        // Any UFE scene notification, we empty the host cache. It will be
        // rebuilt on-demand.
        fromHostCache.clear();
    }
};

Ufe::PathSegment replaceLastComponent(
    const Ufe::PathSegment&             segment,
    const Ufe::PathSegment::Components& components)
{
    auto segmentSize = segment.size();
    if (segmentSize == 1u) {
        // If the segment only had a single component, easy: just create a new
        // segment from the argument components.
        return Ufe::PathSegment(components, segment.runTimeId(), segment.separator());
    }

    // Copy existing components except the last, then append the argument
    // components.
    auto                         nbKeptComponents = segmentSize - 1;
    Ufe::PathSegment::Components newComponents(nbKeptComponents + components.size());

    auto pos = std::copy_n(segment.begin(), nbKeptComponents, newComponents.begin());
    std::copy(components.begin(), components.end(), pos);

    return Ufe::PathSegment(newComponents, segment.runTimeId(), segment.separator());
}

Ufe::Observer::Ptr ufeObserver;

} // namespace

namespace MAYAUSD_NS_DEF {
namespace ufe {

UsdPathMappingHandler::UsdPathMappingHandler()
    : Ufe::PathMappingHandler()
{
}

UsdPathMappingHandler::~UsdPathMappingHandler()
{
    TF_VERIFY(ufeObserver);
    Ufe::Scene::instance().removeObserver(ufeObserver);
    ufeObserver.reset();
}

/*static*/
UsdPathMappingHandler::Ptr UsdPathMappingHandler::create()
{
    TF_VERIFY(!ufeObserver);
    ufeObserver = std::make_shared<UfeObserver>();
    Ufe::Scene::instance().addObserver(ufeObserver);

    return std::make_shared<UsdPathMappingHandler>();
}

//------------------------------------------------------------------------------
// Ufe::PathMappingHandler overrides
//------------------------------------------------------------------------------

Ufe::Path UsdPathMappingHandler::toHost(const Ufe::Path& runTimePath) const { return {}; }

Ufe::Path UsdPathMappingHandler::fromHost(const Ufe::Path& hostPath) const
{
    TF_AXIOM(hostPath.nbSegments() == 1);

    // First look in our cache to see if we've already computed a mapping for the input.
    auto found = fromHostCache.find(hostPath);
    if (found != nullptr) {
        return found->data();
    }

    // If nothing has been pulled, then there is no mapping to be done.
    if (!PXR_NS::PrimUpdaterManager::getInstance().hasPulledPrims()) {
        return {};
    }

    // Start by getting the dag path from the input host path. The dag path is needed
    // to get the pull information (returned as a Ufe::Path) from the plug.
    Ufe::Path mayaHostPath(hostPath);
    auto      dagPath = PXR_NS::UsdMayaUtil::nameToDagPath(hostPath.popHead().string());

    // Iterate over the dagpath (and its ancestors) looking for any pulled info.
    // We keep an array of the Maya node names so we can create a Ufe::PathSegment.
    // We build this array in backwards order (and then reverse it) to maintain a
    // constant time complexity (for addition).
    Ufe::Path                    mayaMappedPath;
    Ufe::PathSegment::Components mayaComps;
    while (dagPath.isValid() && (dagPath.length() > 0)) {
        TF_AXIOM(!mayaHostPath.empty());
        mayaComps.emplace_back(mayaHostPath.back());
        mayaHostPath = mayaHostPath.pop();
        Ufe::Path ufePath;
        if (PXR_NS::PrimUpdaterManager::readPullInformation(dagPath, ufePath)) {
            // From the pulled info path, we pop only the last component and
            // append the Maya component array.
            std::reverse(mayaComps.begin(), mayaComps.end());

            TF_AXIOM(ufePath.nbSegments() == 2);
            const auto& usdSegment = ufePath.getSegments()[1];
            mayaMappedPath = ufePath.popSegment() + replaceLastComponent(usdSegment, mayaComps);

            break;
        }
        dagPath.pop();
    }

    // Store the computed path mapping (can be empty, if none) in our cache.
    fromHostCache.add(hostPath, mayaMappedPath);
    return mayaMappedPath;
}

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
