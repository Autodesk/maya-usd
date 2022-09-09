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

class OrphanedNodesManager : public Ufe::Observer
{
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

public:
    typedef std::shared_ptr<OrphanedNodesManager> Ptr;

    class Memento {
    public:
        
        // Can create an initial empty state, for it to be overwritten later.
        Memento();
        ~Memento() = default;

        Memento(Memento&&);
        Memento& operator=(Memento&&);

        Memento(const Memento&) = delete;
        Memento& operator=(const Memento&) = delete;
        
    private:

        // Private, for opacity.
        friend class OrphanedNodesManager;

        Memento(Ufe::Trie<PullVariantInfo>&& pulledPrims);

        Ufe::Trie<PullVariantInfo> release();

        Ufe::Trie<PullVariantInfo> _pulledPrims;
    };

    OrphanedNodesManager();

    void operator()(const Ufe::Notification&) override;

    // Add the pulled path and its Maya pull parent to the trie of pulled
    // prims.  Asserts that the pulled path is not in the trie.
    void add(const Ufe::Path& pulledPath, const MDagPath& pullParentPath);

    // Remove the pulled path from the trie of pulled prims.  Asserts that the
    // path is in the trie.  Returns a memento (see Memento Pattern) for undo
    // purposes, to be used as argument to restore().
    Memento remove(const Ufe::Path& pulledPath);

    // Restore the trie of pulled prims to the content of the argument memento.
    void restore(Memento&& previous);

    // Clear all pulled paths from the trie of pulled prims.
    void clear();

    // Return true if there are no pulled paths in the trie of pulled prims.
    bool empty() const;

private:

    void handleOp(const Ufe::SceneCompositeNotification::Op& op);

    Ufe::Trie<PullVariantInfo>& pulledPrims();
    const Ufe::Trie<PullVariantInfo>& pulledPrims() const;

    static void
    recursiveSetVisibility(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, bool visibility);
    static void
    recursiveSwitch(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, const Ufe::Path& ufePath);

    static bool
    setVisibilityPlug(const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode, bool visibility);

    // Member function to access private nested classes.
    static std::list<VariantSetDescriptor> variantSetDescriptors(const Ufe::Path& path);

    static Ufe::Trie<PullVariantInfo> deepCopy(const Ufe::Trie<PullVariantInfo>& src);
    static void deepCopy(
        const Ufe::TrieNode<PullVariantInfo>::Ptr& src,
        const Ufe::TrieNode<PullVariantInfo>::Ptr& dst
    );

    // Trie for fast lookup of descendant pulled prims.  The Trie key is the
    // UFE pulled path, and the Trie value is the corresponding Dag pull parent
    // and all ancestor variant set selections.
    Ufe::Trie<PullVariantInfo> _pulledPrims;
};

} // namespace MAYAUSD_NS_DEF
