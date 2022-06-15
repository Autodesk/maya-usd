//
// Copyright 2019 Autodesk
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
#pragma once

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/hash.h>
#include <pxr/base/tf/hashmap.h>
#include <pxr/base/tf/hashset.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MObjectHandle.h>
#include <ufe/path.h>

#include <unordered_map>

// Pending rework of mayaUsd namespaces, MayaUsdProxyShapeBase is in the Pixar
// namespace.  PPT, 9-Mar-2021.
PXR_NAMESPACE_OPEN_SCOPE
class MayaUsdProxyShapeBase;
PXR_NAMESPACE_CLOSE_SCOPE

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief USD Stage Map
/*!
    Two-way map of proxy shape UFE path to corresponding stage.

    We will assume that	a USD proxy shape will not be instanced (even though
    nothing in the data model prevents it).  To generalized access to the
    underlying node, we store an MObjectHandle in the maps.

    The cache is refreshed on access to a stage given a path which cannot be
    found.  In this way, the cache does not need to observe the Maya data
    model, and we avoid order of notification problems where one observer would
    need to access the cache before it is refreshed, since there is no
    guarantee on the order of notification of Ufe observers.  An earlier
    implementation with rename observation had the Maya Outliner (which
    observes rename) access the UsdStageMap on rename before the UsdStageMap
    had been updated.
*/
class MAYAUSD_CORE_PUBLIC UsdStageMap
{
public:
    typedef PXR_NS::TfHashSet<PXR_NS::UsdStageWeakPtr, PXR_NS::TfHash> StageSet;

    UsdStageMap() = default;
    ~UsdStageMap() = default;

    // Delete the copy/move constructors assignment operators.
    UsdStageMap(const UsdStageMap&) = delete;
    UsdStageMap& operator=(const UsdStageMap&) = delete;
    UsdStageMap(UsdStageMap&&) = delete;
    UsdStageMap& operator=(UsdStageMap&&) = delete;

    //! Get USD stage for the first segment of the argument path.
    PXR_NS::UsdStageWeakPtr stage(const Ufe::Path& path);

    //! Return the ProxyShape object for the first segment of the argument
    //! path.  If no such proxy shape exists, returns a null MObject.
    MObject proxyShape(const Ufe::Path& path);

    //! Return the ProxyShape node for the first segment of the argument path.
    //! If no such proxy shape node exists, returns a null pointer.
    PXR_NS::MayaUsdProxyShapeBase* proxyShapeNode(const Ufe::Path& path);

    //! Return the ProxyShape node UFE path for the argument stage.
    Ufe::Path path(PXR_NS::UsdStageWeakPtr stage);

    //! Return all the USD stages.
    StageSet allStages();

    //! Set the stage map as dirty. It will be cleared immediately, but
    //! only repopulated when stage info is requested.
    void setDirty();

    //! Returns true if the stage map is dirty (meaning it needs to be filled in).
    bool isDirty() const { return fDirty; }

private:
    void addItem(const Ufe::Path& path);
    void rebuildCache();
    void rebuildIfDirty();

private:
    // We keep two maps for fast lookup when there are many proxy shapes.
    using PathToObject = std::unordered_map<Ufe::Path, MObjectHandle>;
    using StageToObject = PXR_NS::TfHashMap<PXR_NS::UsdStageWeakPtr, MObjectHandle, PXR_NS::TfHash>;
    PathToObject  fPathToObject;
    StageToObject fStageToObject;
    bool          fDirty { true };

}; // UsdStageMap

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
