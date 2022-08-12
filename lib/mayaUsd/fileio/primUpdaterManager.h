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

#include <maya/MApiNamespace.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MDagPath.h>
#include <ufe/observer.h>
#include <ufe/path.h>
#include <ufe/sceneItem.h>
#include <ufe/sceneNotification.h>
#include <ufe/trie.h>

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

    bool hasPulledPrims() const { return _hasPulledPrims; }

private:
    struct VariantSelection
    {
        VariantSelection() = default;
        VariantSelection(const std::string& vsn, const std::string& vs)
            : variantSetName(vsn)
            , variantSelection(vs)
        {
        }
        bool operator==(const VariantSelection& rhs) const
        {
            return (variantSetName == rhs.variantSetName)
                && (variantSelection == rhs.variantSelection);
        }

        std::string variantSetName;
        std::string variantSelection;
    };

    struct VariantSetDescriptor
    {
        VariantSetDescriptor() = default;
        VariantSetDescriptor(const Ufe::Path& p, const std::list<VariantSelection>& vs)
            : path(p)
            , variantSelections(vs)
        {
        }
        bool operator==(const VariantSetDescriptor& rhs) const
        {
            return (path == rhs.path) && (variantSelections == rhs.variantSelections);
        }

        Ufe::Path                   path;
        std::list<VariantSelection> variantSelections;
    };

    struct PullVariantInfo
    {
        PullVariantInfo() = default;
        PullVariantInfo(const MDagPath& dp, const std::list<VariantSetDescriptor>& vsd)
            : dagPath(dp)
            , variantSetDescriptors(vsd)
        {
        }
        MDagPath                        dagPath;
        std::list<VariantSetDescriptor> variantSetDescriptors;
    };

    PrimUpdaterManager();

    PrimUpdaterManager(PrimUpdaterManager&) = delete;
    PrimUpdaterManager(PrimUpdaterManager&&) = delete;

    bool discardPrimEdits(const Ufe::Path& path);
    bool discardOrphanedEdits(const MDagPath& dagPath);
    void discardPullSetIfEmpty();

    void onProxyContentChanged(const MayaUsdProxyStageObjectsChangedNotice& notice);

    //! Ensure the Dag pull root exists.  This is the child of the Maya world
    //! node under which all pulled nodes are created.  Complexity is O(n) for
    //! n children of the Maya world node.
    MObject findOrCreatePullRoot();

    //! Create the pull parent for the pulled hierarchy.  This is the node
    //! which receives the pulled node's parent transformation.
    MObject createPullParent(const Ufe::Path& pulledPath, MObject pullRoot);

    //! Remove the pull parent for the pulled hierarchy.
    bool removePullParent(const MDagPath& pullParent);

    //! Create the pull parent and set it into the prim updater context.
    MDagPath setupPullParent(const Ufe::Path& pulledPath, VtDictionary& args);

    //! Record pull information for the pulled path, for inspection on
    //! scene changes.
    void recordPullVariantInfo(const Ufe::Path& pulledPath, const MDagPath& pullParentPath);

    // Maya file new or open callback.  Member function to access other private
    // member functions.
    static void beforeNewOrOpenCallback(void* clientData);

    void beginManagePulledPrims();
    void endManagePulledPrims();

    // Member function to access private nested classes.
    static std::list<VariantSetDescriptor> variantSetDescriptors(const Ufe::Path& path);

    friend class TfSingleton<PrimUpdaterManager>;

    class OrphanedNodesManager : public Ufe::Observer
    {
    public:
        OrphanedNodesManager(const Ufe::Trie<PullVariantInfo>& pulledPrims);

        void operator()(const Ufe::Notification&) override;

    private:
        void handleOp(const Ufe::SceneCompositeNotification::Op& op);

        static void recursiveSetVisibility(
            const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
            bool                                       visibility);
        static void recursiveSwitch(
            const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
            const Ufe::Path&                           ufePath);

        static bool
        setVisibilityPlug(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, bool visibility);

        const Ufe::Trie<PullVariantInfo>& _pulledPrims;
    };

    bool _inPushPull { false };

    // Becomes true when there is at least one pulled prim.
    // The goal is to let code that can be optimized when there is no pull prim
    // to check rapidly.
    bool _hasPulledPrims { false };

    // Trie for fast lookup of descendant pulled prims.  The Trie key is the
    // UFE pulled path, and the Trie value is the corresponding Dag pull parent
    // and all ancestor variant set selections.
    Ufe::Trie<PullVariantInfo> _pulledPrims {};

    // Orphaned nodes manager that observes the scene, to determine when to hide
    // pulled prims that have become orphaned, or to show them again, because
    // of structural changes to their USD or Maya ancestors.
    Ufe::Observer::Ptr _orphanedNodesManager {};

    // Maya scene observation, to stop UFE scene observation.
    MCallbackIdArray _fileCbs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
