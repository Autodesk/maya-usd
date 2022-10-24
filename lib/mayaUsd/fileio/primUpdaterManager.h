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
#ifndef PXRUSDMAYA_MAYAPRIMUPDATER_MANAGER_H
#define PXRUSDMAYA_MAYAPRIMUPDATER_MANAGER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/primUpdaterContext.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MCallbackIdArray.h>
#include <ufe/sceneItem.h>

UFE_NS_DEF { class Path; }

#ifdef HAS_ORPHANED_NODES_MANAGER
namespace MAYAUSD_NS_DEF {
class OrphanedNodesManager;
}
#endif

PXR_NAMESPACE_OPEN_SCOPE

class PrimUpdaterManager : public PXR_NS::TfWeakBase
{
public:
    MAYAUSD_CORE_PUBLIC
    ~PrimUpdaterManager();

    MAYAUSD_CORE_PUBLIC
    bool mergeToUsd(
        const MFnDependencyNode& depNodeFn,
        const Ufe::Path&         pulledPath,
        const VtDictionary&      userArgs = VtDictionary());

    MAYAUSD_CORE_PUBLIC
    bool editAsMaya(const Ufe::Path& path, const VtDictionary& userArgs = VtDictionary());

    // Can the prim at the argument path be edited as Maya.
    MAYAUSD_CORE_PUBLIC
    bool canEditAsMaya(const Ufe::Path& path) const;

    MAYAUSD_CORE_PUBLIC
    bool discardEdits(const MDagPath& dagPath);

    MAYAUSD_CORE_PUBLIC
    bool duplicate(
        const Ufe::Path&    srcPath,
        const Ufe::Path&    dstPath,
        const VtDictionary& userArgs = VtDictionary());

    /// \brief Returns the singleton prim updater manager
    MAYAUSD_CORE_PUBLIC
    static PrimUpdaterManager& getInstance();

    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const PXR_NS::UsdPrim& prim, std::string& dagPathStr);
    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const PXR_NS::UsdPrim& prim, Ufe::SceneItem::Ptr& dagPathItem);
    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const Ufe::Path& ufePath, MDagPath& dagPath);
    MAYAUSD_CORE_PUBLIC
    static bool readPullInformation(const MDagPath& dagpath, Ufe::Path& ufePath);

    MAYAUSD_CORE_PUBLIC
    static bool writePulledPrimMetadata(const Ufe::Path& ufePulledPath, const MDagPath& editedRoot);
    MAYAUSD_CORE_PUBLIC
    static bool writePulledPrimMetadata(PXR_NS::UsdPrim& pulledPrim, const MDagPath& editedRoot);
    MAYAUSD_CORE_PUBLIC
    static void removePulledPrimMetadata(const Ufe::Path& ufePulledPath);
    MAYAUSD_CORE_PUBLIC
    static void removePulledPrimMetadata(const UsdStagePtr& stage, UsdPrim& prim);

    MAYAUSD_CORE_PUBLIC
    static bool addExcludeFromRendering(const Ufe::Path& ufePulledPath);
    MAYAUSD_CORE_PUBLIC
    static bool removeExcludeFromRendering(const Ufe::Path& ufePulledPath);

    bool hasPulledPrims() const { return _hasPulledPrims; }

private:
    PrimUpdaterManager();

    PrimUpdaterManager(PrimUpdaterManager&) = delete;
    PrimUpdaterManager(PrimUpdaterManager&&) = delete;

    bool discardPrimEdits(const Ufe::Path& pulledPath);
    bool discardOrphanedEdits(const MDagPath& dagPath, const Ufe::Path& pulledPath);
    void discardPullSetIfEmpty();

    void onProxyContentChanged(const MayaUsdProxyStageObjectsChangedNotice& notice);

    //! Ensure the Dag pull root exists.  This is the child of the Maya world
    //! node under which all pulled nodes are created.  Complexity is O(n) for
    //! n children of the Maya world node.
    MObject findOrCreatePullRoot();

    //! Create the pull parent for the pulled hierarchy.  This is the node
    //! which receives the pulled node's parent transformation.
    MObject createPullParent(const Ufe::Path& pulledPath, MObject pullRoot);

    //! Remove the pull parent for the pulled hierarchy.  Pass in the original
    //! USD pulled path, because at the point of removal of the pull parent the
    //! Maya pulled node no longer exists, and cannot be used to retrieve the
    //! pull information.
    bool removePullParent(const MDagPath& pullParent, const Ufe::Path& pulledPath);

    //! Create the pull parent and set it into the prim updater context.
    MDagPath setupPullParent(const Ufe::Path& pulledPath, VtDictionary& args);

    //! Record pull information for the pulled path, for inspection on
    //! scene changes.
#ifdef HAS_ORPHANED_NODES_MANAGER
    // Maya file new or open callback.  Member function to access other private
    // member functions.
    static void beforeNewOrOpenCallback(void* clientData);

    void beginManagePulledPrims();
    void endManagePulledPrims();

    void beginLoadSaveCallbacks();
    void endLoadSaveCallbacks();

    static void afterNewOrOpenCallback(void* clientData);
    static void beforeSaveCallback(void* clientData);

    void loadOrphanedNodesManagerData();
    void saveOrphanedNodesManagerData();
#endif

    friend class TfSingleton<PrimUpdaterManager>;

    bool _inPushPull { false };

    // Becomes true when there is at least one pulled prim.
    // The goal is to let code that can be optimized when there is no pull prim
    // to check rapidly.
    bool _hasPulledPrims { false };

    // Orphaned nodes manager that observes the scene, to determine when to hide
    // pulled prims that have become orphaned, or to show them again, because
    // of structural changes to their USD or Maya ancestors.
#ifdef HAS_ORPHANED_NODES_MANAGER
    std::shared_ptr<MayaUsd::OrphanedNodesManager> _orphanedNodesManager {};

    // Maya scene observation, to stop UFE scene observation.
    MCallbackIdArray _fileCbs;

    MCallbackIdArray _openSaveCbs;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
