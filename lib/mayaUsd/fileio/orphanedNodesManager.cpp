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
#include "orphanedNodesManager.h"

#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/UsdSceneItem.h>
#include <mayaUsd/ufe/Utils.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usd/editContext.h>

#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>
#include <ufe/hierarchy.h>
#include <ufe/trie.imp.h>

// For Tf diagnostics macros.
PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// Class OrphanedNodesManager::Memento
//------------------------------------------------------------------------------

OrphanedNodesManager::Memento::Memento(Ufe::Trie<PullVariantInfo>&& pulledPrims)
    : _pulledPrims(std::move(pulledPrims))
{
}

OrphanedNodesManager::Memento::Memento()
    : _pulledPrims()
{
}

OrphanedNodesManager::Memento::Memento(Memento&& rhs)
    : _pulledPrims(std::move(rhs._pulledPrims))
{
}

OrphanedNodesManager::Memento& OrphanedNodesManager::Memento::operator=(Memento&& rhs)
{
    _pulledPrims = std::move(rhs._pulledPrims);
    return *this;
}

Ufe::Trie<OrphanedNodesManager::PullVariantInfo> OrphanedNodesManager::Memento::release()
{
    return std::move(_pulledPrims);
}

//------------------------------------------------------------------------------
// Class OrphanedNodesManager
//------------------------------------------------------------------------------

OrphanedNodesManager::OrphanedNodesManager()
    : _pulledPrims()
{
}

void OrphanedNodesManager::add(const Ufe::Path& pulledPath, const MDagPath& pullParentPath)
{
    // Add the pull parent to our pulled prims prefix tree.  Also add the full
    // configuration of variant set selections for each ancestor, up to the USD
    // pseudo-root.  Variants on the pulled path itself are ignored, as once
    // pulled into Maya they cannot be changed.
    TF_AXIOM(!pulledPrims().containsDescendantInclusive(pulledPath));
    TF_AXIOM(pulledPath.runTimeId() == MayaUsd::ufe::getUsdRunTimeId());

    // We store a list of (path, list of (variant set, variant set selection)),
    // for all ancestors, starting at closest ancestor.
    auto ancestorPath = pulledPath.pop();
    auto vsd = variantSetDescriptors(ancestorPath);

    pulledPrims().add(pulledPath, PullVariantInfo(pullParentPath, vsd));
}

OrphanedNodesManager::Memento OrphanedNodesManager::remove(const Ufe::Path& pulledPath)
{
    Memento oldPulledPrims(deepCopy(pulledPrims()));
    TF_AXIOM(pulledPrims().remove(pulledPath) != nullptr);
    return oldPulledPrims;
}

void OrphanedNodesManager::operator()(const Ufe::Notification& n)
{
    const auto& sceneNotification = static_cast<const Ufe::SceneChanged&>(n);
    auto        changedPath = sceneNotification.changedPath();

    // No changedPath means composite.  Use containsDescendant(), as
    // containsDescendantInclusive() would mean a structure change on the
    // pulled node itself, which is not possible (pulled objects are locked).
    if (changedPath.empty()) {
        const auto& sceneCompositeNotification
            = static_cast<const Ufe::SceneCompositeNotification&>(n);
        for (const auto& op : sceneCompositeNotification.opsList()) {
            if (pulledPrims().containsDescendant(op.path)) {
                handleOp(op);
            }
        }
    } else if (pulledPrims().containsDescendant(changedPath)) {
#ifdef UFE_V4_FEATURES_AVAILABLE
        // Use UFE v4 notification to op conversion.
        handleOp(sceneNotification);
#else
        // UFE v3: convert to op ourselves.  Only convert supported
        // notifications.
        if (auto objAdd = dynamic_cast<const Ufe::ObjectAdd*>(&sceneNotification)) {
            handleOp(Ufe::SceneCompositeNotification::Op(
                Ufe::SceneCompositeNotification::ObjectAdd, objAdd->item()));
        } else if (auto objDel = dynamic_cast<const Ufe::ObjectDelete*>(&sceneNotification)) {
            handleOp(Ufe::SceneCompositeNotification::Op(
                Ufe::SceneCompositeNotification::ObjectDelete, objDel->path()));
        } else if (
            auto subtrInv = dynamic_cast<const Ufe::SubtreeInvalidate*>(&sceneNotification)) {
            handleOp(Ufe::SceneCompositeNotification::Op(
                Ufe::SceneCompositeNotification::SubtreeInvalidate, subtrInv->root()));
        }
#endif
    }
}

void OrphanedNodesManager::handleOp(const Ufe::SceneCompositeNotification::Op& op)
{
    switch (op.opType) {
    case Ufe::SceneCompositeNotification::ObjectAdd: {
        // Restoring a previously-deleted scene item may restore an orphaned
        // node.  Traverse the trie, and show hidden pull parents that are
        // descendants of the argument path.  The trie node that corresponds to
        // the added path is the starting point.  It may be an internal node,
        // without data.
        auto ancestorNode = pulledPrims().node(op.path);
        TF_VERIFY(ancestorNode);
        recursiveSetVisibility(ancestorNode, true);
    } break;
    case Ufe::SceneCompositeNotification::ObjectDelete: {
        // The following cases will generate object delete:
        // - Inactivate of ancestor USD prim sends object post delete.  The
        //   inactive object has no children.
        // - Delete of ancestor Maya Dag node, which sends object pre delete.
        //
        // At time of writing (25-Aug-2022), delete of an ancestor USD prim
        // (which sends object destroyed) is prevented by edit restrictions, as
        // pulling creates an over opinion all along the ancestor chain in the
        // session layer, which is strongest.  If these restrictions are
        // lifted, hiding the pull parent is appropriate.
        //
        // Traverse the trie, and hide pull parents that are descendants of
        // the argument path.  First, get the trie node that corresponds to
        // the path.  It may be an internal node, without data.
        auto ancestorNode = pulledPrims().node(op.path);
        TF_VERIFY(ancestorNode);
        recursiveSetVisibility(ancestorNode, false);
    } break;
    case Ufe::SceneCompositeNotification::SubtreeInvalidate: {
        // On subtree invalidate, the scene item itself has not had a structure
        // change, but its children have changed.  There are two cases:
        // - the node has children: from a variant switch, or from a payload
        //   load.
        // - the node has no children: from a payload unload.
        // In the latter case, call recursiveHide(), because there is nothing
        // below the invalidated node.

        auto parentItem = Ufe::Hierarchy::createItem(op.path);
        if (!TF_VERIFY(parentItem)) {
            return;
        }

        auto parentHier = Ufe::Hierarchy::hierarchy(parentItem);
        if (!parentHier->hasChildren()) {
            auto ancestorNode = pulledPrims().node(op.path);
            if (ancestorNode) {
                recursiveSetVisibility(ancestorNode, false);
            }
            return;
        } else {
            // On variant switch, given a pulled prim, the session layer will
            // have path-based USD overs for pull information and active
            // (false) for that prim in the session layer.  If a prim child
            // brought in by variant switching has the same name as that of the
            // pulled prim in a previous variant, the overs will apply to to
            // the new prim, which would then get a path mapping, which is
            // inappropriate.  Read children using the USD API, including
            // inactive children (since pulled prims are inactivated), to
            // support a variant switch to variant child with the same name.

            auto parentUsdItem = std::dynamic_pointer_cast<MayaUsd::ufe::UsdSceneItem>(parentItem);
            if (!parentUsdItem) {
                // USD sends resync changes (UFE subtree invalidate) on the
                // pseudo-root itself.  Since the pseudo-root has no payload or
                // variant, ignore these.
                TF_AXIOM(parentItem->path().nbSegments() == 1);
                return;
            }
            auto parentPrim = parentUsdItem->prim();
            bool foundChild { false };
            for (const auto& c :
                 parentPrim.GetFilteredChildren(UsdPrimIsDefined && !UsdPrimIsAbstract)) {
                auto cPath = parentItem->path().popSegment();
                cPath = cPath
                    + Ufe::PathSegment(
                            c.GetPath().GetAsString(), MayaUsd::ufe::getUsdRunTimeId(), '/');

                auto ancestorNode = pulledPrims().node(cPath);
                // If there is no ancestor node in the trie, this means that
                // the new hierarchy is completely different from the one when
                // the pull occurred, which means that the pulled object must
                // stay hidden.
                if (ancestorNode) {
                    foundChild = true;
                    recursiveSwitch(ancestorNode, cPath);
                }
            }
            if (!foundChild) {
                // Following a subtree invalidate, if none of the now-valid
                // children appear in the trie, means that we've switched to a
                // different variant, and everything below that path should be
                // hidden.
                auto ancestorNode = pulledPrims().node(op.path);
                if (ancestorNode) {
                    recursiveSetVisibility(ancestorNode, false);
                }
            }
        }
    } break;
    default: {
        // ObjectPathChange (reparent, rename): to be implemented (MAYA-125039).
        // SceneCompositeNotification: already expanded in operator().
    }
    }
}

Ufe::Trie<OrphanedNodesManager::PullVariantInfo>& OrphanedNodesManager::pulledPrims()
{
    return _pulledPrims;
}

const Ufe::Trie<OrphanedNodesManager::PullVariantInfo>& OrphanedNodesManager::pulledPrims() const
{
    return _pulledPrims;
}

void OrphanedNodesManager::clear() { pulledPrims().clear(); }

bool OrphanedNodesManager::empty() const { return pulledPrims().root()->empty(); }

void OrphanedNodesManager::restore(Memento&& previous) { _pulledPrims = previous.release(); }

/* static */
bool OrphanedNodesManager::setVisibilityPlug(
    const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
    bool                                       visibility)
{
    TF_VERIFY(trieNode->hasData());
    const auto& pullParentPath = trieNode->data().dagPath;
    MFnDagNode  fn(pullParentPath);
    auto        visibilityPlug = fn.findPlug("visibility", /* tryNetworked */ true);
    return (visibilityPlug.setBool(visibility) == MS::kSuccess);
}

/* static */
void OrphanedNodesManager::recursiveSetVisibility(
    const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
    bool                                       visibility)
{
    // We know in our case that a trie node with data can't have children,
    // since descendants of a pulled prim can't be pulled.
    if (trieNode->hasData()) {
        TF_VERIFY(trieNode->empty());
        TF_VERIFY(setVisibilityPlug(trieNode, visibility));
    } else {
        auto childrenComponents = trieNode->childrenComponents();
        for (const auto& c : childrenComponents) {
            recursiveSetVisibility((*trieNode)[c], visibility);
        }
    }
}

/* static */
void OrphanedNodesManager::recursiveSwitch(
    const Ufe::TrieNode<PullVariantInfo>::Ptr& trieNode,
    const Ufe::Path&                           ufePath)
{
    // We know in our case that a trie node with data can't have children,
    // since descendants of a pulled prim can't be pulled.  A trie node with
    // data is one that's been pulled.
    if (trieNode->hasData()) {
        TF_VERIFY(trieNode->empty());

        auto pulledNode = std::dynamic_pointer_cast<MayaUsd::ufe::UsdSceneItem>(
            Ufe::Hierarchy::createItem(ufePath));
        if (!TF_VERIFY(pulledNode)) {
            return;
        }

        // If the variant set configuration of the pulled node and the current
        // tree state don't match, the pulled node must be made invisible.
        // Inactivation must not be considered, as the USD pulled node is made
        // inactive on pull, to avoid rendering it.
        const bool variantSetsMatch
            = (trieNode->data().variantSetDescriptors == variantSetDescriptors(ufePath.pop()));
        const bool visibility = (pulledNode && variantSetsMatch);
        TF_VERIFY(setVisibilityPlug(trieNode, visibility));

        // Set the activation of the pulled USD prim to the opposite of that of
        // the corresponding Maya node: other variants may refer to the same
        // path, and we don't want those paths to hit an inactive prim.  No
        // need to remove an inert primSpec: this will be done on push.
        auto prim = MayaUsd::ufe::ufePathToPrim(ufePath);
        auto stage = prim.GetStage();
        if (TF_VERIFY(stage)) {
            UsdEditContext editContext(stage, stage->GetSessionLayer());
            TF_VERIFY(prim.SetActive(!visibility));
        }
    } else {
        auto childrenComponents = trieNode->childrenComponents();
        for (const auto& c : childrenComponents) {
            auto childTrieNode = (*trieNode)[c];
            if (childTrieNode) {
                recursiveSwitch((*trieNode)[c], ufePath + c);
            }
        }
    }
}

/* static */
std::list<OrphanedNodesManager::VariantSetDescriptor>
OrphanedNodesManager::variantSetDescriptors(const Ufe::Path& p)
{
    std::list<VariantSetDescriptor> vsd;
    auto                            path = p;
    while (path.runTimeId() == MayaUsd::ufe::getUsdRunTimeId()) {
        auto ancestor = Ufe::Hierarchy::createItem(path);
        auto usdAncestor = std::static_pointer_cast<MayaUsd::ufe::UsdSceneItem>(ancestor);
        auto variantSets = usdAncestor->prim().GetVariantSets();
        std::list<VariantSelection> vs;
        for (const auto& vsn : variantSets.GetNames()) {
            vs.emplace_back(vsn, variantSets.GetVariantSelection(vsn));
        }
        vsd.emplace_back(path, vs);
        path = path.pop();
    }
    return vsd;
}

/* static */
Ufe::Trie<OrphanedNodesManager::PullVariantInfo>
OrphanedNodesManager::deepCopy(const Ufe::Trie<PullVariantInfo>& src)
{
    Ufe::Trie<PullVariantInfo> dst;
    deepCopy(src.root(), dst.root());
    return dst;
}

/* static */
void OrphanedNodesManager::deepCopy(
    const Ufe::TrieNode<PullVariantInfo>::Ptr& src,
    const Ufe::TrieNode<PullVariantInfo>::Ptr& dst)
{
    for (const auto& c : src->childrenComponents()) {
        const auto& srcChild = (*src)[c];
        auto        dstChild = std::make_shared<Ufe::TrieNode<PullVariantInfo>>(c);
        dst->add(dstChild);
        if (srcChild->hasData()) {
            dstChild->setData(srcChild->data());
        }
        deepCopy(srcChild, dstChild);
    }
}

} // namespace MAYAUSD_NS_DEF
