//
// Copyright 2022 Autodesk
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

#include <maya/MDagPath.h>
#include <ufe/observer.h>
#include <ufe/path.h>
#include <ufe/sceneNotification.h>
#include <ufe/trie.h>

namespace MAYAUSD_NS_DEF {

/// \class OrphanedNodesManager
///
/// \brief Records the data that affects which exact USD prim was edited as Maya.
///
/// Prims edited as Maya nodes are only valid if the prim is still accessible
/// in the USD stage. If no longer accessible, we declared the Maya nodes as
/// orphaned and hide them.
///
/// Observes the scene, to determine when to hide edited prims that have become
/// orphaned, or to show them again, because of structural changes to their USD
/// or Maya ancestors.
///
/// Currently, the only state that we monitor and consider for prim validity
/// and edit orphaning is the set of variant selections of all ancestors of
/// the prim being edited.

class OrphanedNodesManager : public Ufe::Observer
{
public:
    /// \brief Records a single variant selection of a single variant set.
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

    /// \brief Records all variant selections of a single prim.
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

    /// \brief Records all variant selections of all ancestors of the prim edited as maya,
    ///        with the DAG path of the root of Maya nodes corresponding to the edited prim.
    struct PullVariantInfo
    {
        PullVariantInfo() = default;
        PullVariantInfo(
            const MDagPath&                        proxyShape,
            const MDagPath&                        pulledParent,
            const MDagPath&                        editedMayaRoot,
            const std::list<VariantSetDescriptor>& vsd)
            : proxyShapePath(proxyShape)
            , pulledParentPath(pulledParent)
            , editedAsMayaRoot(editedMayaRoot)
            , variantSetDescriptors(vsd)
        {
        }
        MDagPath                        proxyShapePath;
        MDagPath                        pulledParentPath;
        MDagPath                        editedAsMayaRoot;
        std::list<VariantSetDescriptor> variantSetDescriptors;
    };

    /// \brief Entire state of the OrphanedNodesManager at a point in time, used for undo/redo.
    class Memento
    {
    public:
        // Can create an initial empty state, for it to be overwritten later.
        Memento();
        ~Memento() = default;

        Memento(Memento&&);
        Memento& operator=(Memento&&);

        Memento(const Memento&) = delete;
        Memento& operator=(const Memento&) = delete;

        static std::string convertToJson(const Memento&);
        static Memento     convertFromJson(const std::string&);

    private:
        // Private, for opacity.
        friend class OrphanedNodesManager;

        Memento(Ufe::Trie<PullVariantInfo>&& pulledPrims);

        Ufe::Trie<PullVariantInfo> release();

        Ufe::Trie<PullVariantInfo> _pulledPrims;
    };

    // Construct an empty orphan manager.
    OrphanedNodesManager();

    // Notifications handling, part of the Ufe::Observer interface.
    void operator()(const Ufe::Notification&) override;

    // Add the pulled path, its Maya pull parent and the root of the generated
    // Maya nodes to the trie of pulled prims.
    // Asserts that the pulled path is not in the trie.
    void
    add(const Ufe::Path& pulledPath,
        const MDagPath&  pullParentPath,
        const MDagPath&  editedAsMayaRoot);

    // Remove the pulled path from the trie of pulled prims.  Asserts that the
    // path is in the trie.  Returns a memento (see Memento Pattern) for undo
    // purposes, to be used as argument to restore().
    Memento remove(const Ufe::Path& pulledPath);

    // Preserve the trie of pulled prims into a memento.
    Memento preserve() const;

    // Restore the trie of pulled prims to the content of the argument memento.
    void restore(Memento&& previous);

    // Clear all pulled paths from the trie of pulled prims.
    void clear();

    // Return true if there are no pulled paths in the trie of pulled prims.
    bool empty() const;

    // Return whether the Dag hierarchy corresponding to the pulled path is
    // orphaned.
    bool isOrphaned(const Ufe::Path& pulledPath) const;

private:
    void handleOp(const Ufe::SceneCompositeNotification::Op& op);

    Ufe::Trie<PullVariantInfo>&       pulledPrims();
    const Ufe::Trie<PullVariantInfo>& pulledPrims() const;

    static void
    recursiveSetOrphaned(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, bool orphaned);
    static void
    recursiveSwitch(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, const Ufe::Path& ufePath);

    static bool setOrphaned(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, bool orphaned);

    // Member function to access private nested classes.
    static std::list<VariantSetDescriptor> variantSetDescriptors(const Ufe::Path& path);

    static Ufe::Trie<PullVariantInfo> deepCopy(const Ufe::Trie<PullVariantInfo>& src);
    static void                       deepCopy(
                              const Ufe::TrieNode<PullVariantInfo>::Ptr& src,
                              const Ufe::TrieNode<PullVariantInfo>::Ptr& dst);

    // Trie for fast lookup of descendant pulled prims.  The Trie key is the
    // UFE pulled path, and the Trie value is the corresponding Dag pull parent
    // and all ancestor variant set selections.
    Ufe::Trie<PullVariantInfo> _pulledPrims;
};

} // namespace MAYAUSD_NS_DEF
