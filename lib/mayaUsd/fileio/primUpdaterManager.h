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
#include <mayaUsd/fileio/pullInformation.h>
#include <mayaUsd/listeners/proxyShapeNotice.h>

#include <pxr/base/tf/registryManager.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/prim.h>

#include <maya/MCallbackIdArray.h>

UFE_NS_DEF { class Path; }

#ifdef HAS_ORPHANED_NODES_MANAGER
namespace MAYAUSD_NS_DEF {
class OrphanedNodesManager;
}
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Arguments used by PrimUpdaterManager pushToUsd operations mergeToUsd and duplicateToUsd.
struct MAYAUSD_CORE_PUBLIC PushToUsdArgs
{
    /// The source maya object to push to USD.
    const MObject srcMayaObject;
    /// The UFE path to the destination USD prim.
    const Ufe::Path dstUfePath;
    /// User arguments dictionary.
    const VtDictionary userArgs;
    /// Arguments that confirure the updaters.
    const UsdMayaPrimUpdaterArgs updaterArgs;

    /// \brief Creates PushToUsdArgs for mergeToUsd. Given \p dagPath will be pushed to its
    /// pulled USD prim with given \p usrArgs export args.
    static PushToUsdArgs forMerge(const MDagPath& dagPath, const VtDictionary& usrArgs = {});

    /// \brief Creates PushToUsdArgs for duplicateToUsd. The maya node \p object will be
    /// duplicated to USD prim at \p dstPath with given \p usrArgs export args.
    static PushToUsdArgs
    forDuplicate(const MObject& object, const Ufe::Path& dstPath, const VtDictionary& usrArgs = {});

    /// Are all required fields valid.
    explicit operator bool() const;

protected:
    PushToUsdArgs();
    PushToUsdArgs(const MObject& mayaObject, const Ufe::Path& ufePath, VtDictionary&& userArgs);
};

class PrimUpdaterManager : public PXR_NS::TfWeakBase
{
public:
    MAYAUSD_CORE_PUBLIC
    MAYAUSD_DISALLOW_COPY_MOVE_AND_ASSIGNMENT(PrimUpdaterManager);
    ~PrimUpdaterManager();

    /// \brief merges multiple edited Maya data into their corresponding USD stages.
    MAYAUSD_CORE_PUBLIC
    std::vector<Ufe::Path> mergeToUsd(const std::vector<PushToUsdArgs>& mergeArgsVect);

    /// \brief edit USD data as Maya data.
    MAYAUSD_CORE_PUBLIC
    bool editAsMaya(const Ufe::Path& path, const VtDictionary& userArgs = VtDictionary());

    /// \brief Verify if the prim at the argument path be edited as Maya.
    MAYAUSD_CORE_PUBLIC
    bool canEditAsMaya(const Ufe::Path& path) const;

    /// \brief discards edited Maya data.
    MAYAUSD_CORE_PUBLIC
    bool discardEdits(const MDagPath& dagPath);

    /// \brief Copy Maya nodes to USD data or USD data to Maya nodes.
    /// \return list of destination paths.
    MAYAUSD_CORE_PUBLIC
    std::vector<Ufe::Path> duplicate(
        const Ufe::Path&    srcPath,
        const Ufe::Path&    dstPath,
        const VtDictionary& userArgs = VtDictionary());

    /// \brief Copy USD data to Maya nodes.
    /// \return list of destination paths.
    MAYAUSD_CORE_PUBLIC
    std::vector<Ufe::Path> duplicateToMaya(
        const Ufe::Path&    srcPath,
        const Ufe::Path&    dstPath,
        const VtDictionary& userArgs = VtDictionary());

    /// \brief Copy Maya nodes to USD data.
    /// \return list of destination paths.
    MAYAUSD_CORE_PUBLIC
    std::vector<Ufe::Path> duplicateToUsd(
        const MObject&      mayaObject,
        const Ufe::Path&    dstPath,
        const VtDictionary& userArgs = VtDictionary());

    /// \brief Returns the singleton prim updater manager
    MAYAUSD_CORE_PUBLIC
    static PrimUpdaterManager& getInstance();

    /// \brief Verify if there are any data edited as Maya data.
    MAYAUSD_CORE_PUBLIC
    bool hasPulledPrims() const;

    using PulledPrimPaths = std::vector<std::pair<Ufe::Path, MDagPath>>;

    /// \brief Retrieve the UFE path of the edited USD data and the corresponding path of Maya data.
    MAYAUSD_CORE_PUBLIC
    PulledPrimPaths getPulledPrimPaths() const;

private:
    PrimUpdaterManager();

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

    //! Verify if the given prim at the given UFE path is an ancestor of an already edited prim.
    bool hasEditedDescendant(const Ufe::Path& ufeQueryPath) const;

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
