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

#include <mayaUsd/fileio/primUpdaterManager.h>
#include <mayaUsd/fileio/pullInformation.h>
#include <mayaUsd/nodes/proxyShapeBase.h>
#include <mayaUsd/ufe/Global.h>
#include <mayaUsd/ufe/Utils.h>

#include <usdUfe/ufe/UsdSceneItem.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/usd/usd/editContext.h>

#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>
#include <ufe/hierarchy.h>
#include <ufe/sceneSegmentHandler.h>
#include <ufe/trie.imp.h>

// For Tf diagnostics macros.
PXR_NAMESPACE_USING_DIRECTIVE

namespace MAYAUSD_NS_DEF {

//------------------------------------------------------------------------------
// Class OrphanedNodesManager::Memento
//------------------------------------------------------------------------------

OrphanedNodesManager::Memento::Memento(PulledPrims&& pulledPrims)
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

namespace {

using PullVariantInfo = OrphanedNodesManager::PullVariantInfo;
using VariantSetDescriptor = OrphanedNodesManager::VariantSetDescriptor;
using VariantSelection = OrphanedNodesManager::VariantSelection;
using PulledPrims = OrphanedNodesManager::PulledPrims;
using PulledPrimNode = OrphanedNodesManager::PulledPrimNode;

Ufe::Path trieNodeToPulledPrimUfePath(PulledPrimNode::Ptr trieNode);

void renameVariantDescriptors(
    std::list<VariantSetDescriptor>& descriptors,
    const Ufe::Path&                 oldPath,
    const Ufe::Path&                 newPath)
{
    std::list<VariantSetDescriptor> newDescriptors;
    for (VariantSetDescriptor& desc : descriptors) {
        if (desc.path.startsWith(oldPath)) {
            desc.path = desc.path.reparent(oldPath, newPath);
        }
    }
}

void renameVariantInfo(
    const PulledPrimNode::Ptr& trieNode,
    const Ufe::Path&           oldPath,
    const Ufe::Path&           newPath)
{
    // Note: TrieNode has no non-const data() function, so to modify the
    //       data we must make a copy, modify the copy and call setData().
    PullVariantInfo newVariantInfo = trieNode->data();

    // Note: the change to USD data must be done *after* changes to Maya data because
    //       the outliner reacts to UFE notifications received following the USD edits
    //       to rebuild the node tree and the Maya node we want to hide must have been
    //       hidden by that point. So the node visibility change must be done *first*.
    renameVariantDescriptors(newVariantInfo.variantSetDescriptors, oldPath, newPath);

    Ufe::Path pulledPath = trieNodeToPulledPrimUfePath(trieNode);
    TF_VERIFY(writePullInformation(pulledPath, newVariantInfo.editedAsMayaRoot));

    trieNode->setData(newVariantInfo);
}

void recursiveRename(
    const PulledPrimNode::Ptr& trieNode,
    const Ufe::Path&           oldPath,
    const Ufe::Path&           newPath)
{
    if (trieNode->hasData()) {
        renameVariantInfo(trieNode, oldPath, newPath);
    } else {
        auto childrenComponents = trieNode->childrenComponents();
        for (auto& c : childrenComponents) {
            recursiveRename((*trieNode)[c], oldPath, newPath);
        }
    }
}

void handlePathChange(
    const Ufe::Path&           oldPath,
    const Ufe::SceneItem::Ptr& item,
    PulledPrims&               pulledPrims)
{
    if (!item)
        return;

    auto trieNode = pulledPrims.node(oldPath);
    if (trieNode) {
        const Ufe::Path& newPath = item->path();
        // If the only change is the last part of the UFE path, then
        // we are dealing with a rename. Else it is a reparent.
        if (newPath.pop() == oldPath.pop()) {
            trieNode->rename(newPath.back());
        } else {
            pulledPrims.move(oldPath, newPath);
        }
        recursiveRename(trieNode, oldPath, newPath);
    }
}

// Control the orphaned nodes manager in-orphaning flag.
class Orphaning
{
public:
    Orphaning(int& orphaning)
        : _orphaning(orphaning)
    {
        ++_orphaning;
    }

    ~Orphaning() { --_orphaning; }

private:
    int& _orphaning;
};

} // namespace

OrphanedNodesManager::OrphanedNodesManager()
    : _pulledPrims()
{
}

void OrphanedNodesManager::add(const Ufe::Path& pulledPath, const MDagPath& editedAsMayaRoot)
{
    // Add the edited-as-Maya root to our pulled prims prefix tree.  Also add the full
    // configuration of variant set selections for each ancestor, up to the USD
    // pseudo-root.  Variants on the pulled path itself are ignored, as once
    // pulled into Maya they cannot be changed.
    TF_AXIOM(!_pulledPrims.containsDescendantInclusive(pulledPath));
    TF_AXIOM(pulledPath.runTimeId() == MayaUsd::ufe::getUsdRunTimeId());

    // We store a list of (path, list of (variant set, variant set selection)),
    // for all ancestors, starting at closest ancestor.
    auto ancestorPath = pulledPath.pop();
    auto vsd = variantSetDescriptors(ancestorPath);

    _pulledPrims.add(pulledPath, PullVariantInfo(editedAsMayaRoot, vsd));
}

OrphanedNodesManager::Memento OrphanedNodesManager::remove(const Ufe::Path& pulledPath)
{
    Memento oldPulledPrims(preserve());
    TF_AXIOM(_pulledPrims.remove(pulledPath) != nullptr);
    return oldPulledPrims;
}

const PullVariantInfo& OrphanedNodesManager::get(const Ufe::Path& pulledPath) const
{
    const auto infoNode = _pulledPrims.find(pulledPath);
    if (!infoNode || !infoNode->hasData()) {
        static const PullVariantInfo empty;
        return empty;
    }

    return infoNode->data();
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
            if (_pulledPrims.containsDescendant(op.path)) {
                handleOp(op);
            }
        }
    } else if (_pulledPrims.containsDescendant(changedPath)) {
#ifdef UFE_V4_FEATURES_AVAILABLE
        // Use UFE v4 notification to op conversion.
        handleOp(sceneNotification);
#else
        // UFE v3: convert to op ourselves.  Only convert supported
        // notifications.
        if (auto objAdd = dynamic_cast<const Ufe::ObjectAdd*>(&sceneNotification)) {
            handleOp(Ufe::SceneCompositeNotification::Op(
                Ufe::SceneCompositeNotification::OpType::ObjectAdd, objAdd->item()));
        } else if (auto objDel = dynamic_cast<const Ufe::ObjectDelete*>(&sceneNotification)) {
            handleOp(Ufe::SceneCompositeNotification::Op(
                Ufe::SceneCompositeNotification::OpType::ObjectDelete, objDel->path()));
        } else if (
            auto subtrInv = dynamic_cast<const Ufe::SubtreeInvalidate*>(&sceneNotification)) {
            handleOp(Ufe::SceneCompositeNotification::Op(
                Ufe::SceneCompositeNotification::OpType::SubtreeInvalidate, subtrInv->root()));
        } else if (auto objRename = dynamic_cast<const Ufe::ObjectRename*>(&sceneNotification)) {
            handlePathChange(objRename->previousPath(), objRename->item(), _pulledPrims);
        } else if (auto objRep = dynamic_cast<const Ufe::ObjectReparent*>(&sceneNotification)) {
            handlePathChange(objRep->previousPath(), objRep->item(), _pulledPrims);
        }
#endif
    }
}

void OrphanedNodesManager::handleOp(const Ufe::SceneCompositeNotification::Op& op)
{
    if (_inOrphaning > 0)
        return;

    Orphaning orphaning(_inOrphaning);

    switch (op.opType) {
    case Ufe::SceneCompositeNotification::OpType::ObjectAdd: {
        // Restoring a previously-deleted scene item may restore an orphaned
        // node.  Traverse the trie, and show hidden pull parents that are
        // descendants of the argument path that have all the proper variants.
        // The trie node that corresponds to the added path is the starting
        // point.  It may be an internal node, without data.
        auto ancestorNode = _pulledPrims.node(op.path);
        TF_VERIFY(ancestorNode);
        recursiveSwitch(ancestorNode, op.path);
    } break;
    case Ufe::SceneCompositeNotification::OpType::ObjectDelete: {
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
        auto ancestorNode = _pulledPrims.node(op.path);
        TF_VERIFY(ancestorNode);
        recursiveSetOrphaned(ancestorNode, true);
    } break;
    case Ufe::SceneCompositeNotification::OpType::SubtreeInvalidate: {
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
            auto ancestorNode = _pulledPrims.node(op.path);
            if (ancestorNode) {
                recursiveSetOrphaned(ancestorNode, true);
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

            auto parentUsdItem = std::dynamic_pointer_cast<UsdUfe::UsdSceneItem>(parentItem);
            if (!parentUsdItem) {
                // USD sends resync changes (UFE subtree invalidate) on the
                // pseudo-root itself.  Since the pseudo-root has no payload or
                // variant, ignore these.
                TF_AXIOM(parentItem->path().nbSegments() == 1);
                return;
            }
            auto parentPrim = parentUsdItem->prim();
            bool foundChild { false };
            for (const auto& child :
                 parentPrim.GetFilteredChildren(UsdPrimIsDefined && !UsdPrimIsAbstract)) {
                auto childPath = parentItem->path().popSegment();
                childPath
                    = childPath
                    + Ufe::PathSegment(
                          child.GetPath().GetAsString(), MayaUsd::ufe::getUsdRunTimeId(), '/');

                auto ancestorNode = _pulledPrims.node(childPath);
                // If there is no ancestor node in the trie, this means that
                // the new hierarchy is completely different from the one when
                // the pull occurred, which means that the pulled object must
                // stay hidden.
                if (!ancestorNode)
                    continue;

                foundChild = true;
                recursiveSwitch(ancestorNode, childPath);
            }
            if (!foundChild) {
                // Following a subtree invalidate, if none of the now-valid
                // children appear in the trie, means that we've switched to a
                // different variant, and everything below that path should be
                // hidden.
                auto ancestorNode = _pulledPrims.node(op.path);
                if (ancestorNode) {
                    recursiveSetOrphaned(ancestorNode, true);
                }
            }
        }
    } break;
#ifdef UFE_V4_FEATURES_AVAILABLE
    case Ufe::SceneCompositeNotification::OpType::ObjectPathChange: {
        if (op.subOpType == Ufe::ObjectPathChange::ObjectRename
            || op.subOpType == Ufe::ObjectPathChange::ObjectReparent) {
            handlePathChange(op.path, op.item, _pulledPrims);
        }
    } break;
#endif
    default: {
        // SceneCompositeNotification: already expanded in operator().
    }
    }
}

void OrphanedNodesManager::clear() { _pulledPrims.clear(); }

bool OrphanedNodesManager::empty() const { return _pulledPrims.root()->empty(); }

OrphanedNodesManager::Memento OrphanedNodesManager::preserve() const
{
    return Memento(deepCopy(_pulledPrims));
}

void OrphanedNodesManager::restore(Memento&& previous) { _pulledPrims = previous.release(); }

bool OrphanedNodesManager::isOrphaned(const Ufe::Path& pulledPath) const
{
    auto trieNode = _pulledPrims.node(pulledPath);
    if (!trieNode) {
        // If the argument path has not been pulled, it can't be orphaned.
        return false;
    }

    if (!trieNode->hasData()) {
        // If the argument path has not been pulled, it can't be orphaned.
        return false;
    }

    const PullVariantInfo& variantInfo = trieNode->data();

    // If the pull parent is visible, the pulled path is not orphaned.
    MDagPath pullParentPath = variantInfo.editedAsMayaRoot;
    pullParentPath.pop();

    MFnDagNode fn(pullParentPath);
    auto       visibilityPlug = fn.findPlug("visibility", /* tryNetworked */ true);
    return !visibilityPlug.asBool();
}

namespace {

Ufe::Path
trieNodeToPulledPrimUfePath(Ufe::TrieNode<OrphanedNodesManager::PullVariantInfo>::Ptr trieNode)
{
    // Accumulate all UFE path components, in reverse order. We will pop them
    // from the back while building the pulled prim path.
    //
    // Note: the trie root node is not really part of the hierarchy, so do not
    //       include it in the components. We detect we are at the root when
    //       the node has no parent.
    Ufe::PathSegment::Components pathComponents;
    while (trieNode->parent()) {
        pathComponents.push_back(trieNode->component());
        trieNode = trieNode->parent();
    }

    // We assume the prim path is comosed of two segments: one in Maya, up to the
    // stage proxy shape, then in USD.
    Ufe::Path primPath;
    bool      foundStage = false;

    while (pathComponents.size() > 0) {
        Ufe::PathComponent comp = pathComponents.back();
        pathComponents.pop_back();

        // If the path is empty, it means we are starting the Maya path, so create
        // a Maya UFE segment.
        //
        // Note: the reason we don't just create an empty segment right away when
        //       creating the UFE path is that the + operator refuses to add a
        //       component if there are zero component in the path. So we create
        //       the Maya segment when we extract the first component. That also
        //       avoids duplicating the code to check if we found a stage, just below.
        if (primPath.empty()) {
            primPath = primPath + Ufe::PathSegment(comp, ufe::getMayaRunTimeId(), '|');
        } else {
            primPath = primPath + comp;
        }

        // If we have net found the stage proxy node in Maya, check if the
        // path matches any stage and create the USD segment once we do find
        // a matching stage.
        if (!foundStage) {
            UsdStagePtr stage = ufe::getStage(primPath);
            if (stage) {
                primPath = primPath
                    + Ufe::PathSegment(Ufe::PathSegment::Components(), ufe::getUsdRunTimeId(), '/');
                foundStage = true;
            }
        }
    }

    // If we did not find a stage, it means the stage was deleted,
    // so return an empty path instead of a path to nowhere.
    return foundStage ? primPath : Ufe::Path();
}

MStatus setNodeVisibility(const MDagPath& dagPath, bool visibility)
{
    MFnDagNode fn(dagPath);
    auto       visibilityPlug = fn.findPlug("visibility", /* tryNetworked */ true);
    return visibilityPlug.setBool(visibility);
}

} // namespace

/* static */
bool OrphanedNodesManager::setOrphaned(const PulledPrimNode::Ptr& trieNode, bool orphaned)
{
    TF_VERIFY(trieNode->hasData());

    const PullVariantInfo& variantInfo = trieNode->data();

    // Note: the change to USD data must be done *after* changes to Maya data because
    //       the outliner reacts to UFE notifications received following the USD edits
    //       to rebuild the node tree and the Maya node we want to hide must have been
    //       hidden by that point. So the node visibility change must be done *first*.
    MDagPath pullParentPath = variantInfo.editedAsMayaRoot;
    pullParentPath.pop();
    CHECK_MSTATUS_AND_RETURN(setNodeVisibility(pullParentPath, !orphaned), false);

    const Ufe::Path pulledPrimPath = trieNodeToPulledPrimUfePath(trieNode);

    // Note: if we are called due to the user deleting the stage, then the pulled prim
    //       path will be invalid and trying to add or remove information on it will
    //       fail, and cause spurious warnings in the script editor, so avoid it.
    if (!pulledPrimPath.empty()) {
        if (orphaned) {
            removePulledPrimMetadata(pulledPrimPath);
            removeExcludeFromRendering(pulledPrimPath);
        } else {
            writePulledPrimMetadata(pulledPrimPath, variantInfo.editedAsMayaRoot);
            addExcludeFromRendering(pulledPrimPath);
        }
    }

    return true;
}

/* static */
void OrphanedNodesManager::recursiveSetOrphaned(const PulledPrimNode::Ptr& trieNode, bool orphaned)
{
    // We know in our case that a trie node with data can't have children,
    // since descendants of a pulled prim can't be pulled.
    if (trieNode->hasData()) {
        TF_VERIFY(trieNode->empty());
        TF_VERIFY(setOrphaned(trieNode, orphaned));
    } else {
        auto childrenComponents = trieNode->childrenComponents();
        for (const auto& c : childrenComponents) {
            recursiveSetOrphaned((*trieNode)[c], orphaned);
        }
    }
}

/* static */
void OrphanedNodesManager::recursiveSwitch(
    const PulledPrimNode::Ptr& trieNode,
    const Ufe::Path&           ufePath)
{
    // We know in our case that a trie node with data can't have children,
    // since descendants of a pulled prim can't be pulled.  A trie node with
    // data is one that's been pulled.
    if (trieNode->hasData()) {
        TF_VERIFY(trieNode->empty());

        auto pulledNode
            = std::dynamic_pointer_cast<UsdUfe::UsdSceneItem>(Ufe::Hierarchy::createItem(ufePath));
        if (!TF_VERIFY(pulledNode)) {
            return;
        }

        // If the variant set configuration of the pulled node and the current
        // tree state don't match, the pulled node must be made invisible.
        // Inactivation must not be considered, as the USD pulled node is made
        // inactive on pull, to avoid rendering it.
        const auto& originalDesc = trieNode->data().variantSetDescriptors;
        const auto  currentDesc = variantSetDescriptors(ufePath.pop());
        const bool  variantSetsMatch = (originalDesc == currentDesc);
        const bool  orphaned = (pulledNode && !variantSetsMatch);
        TF_VERIFY(setOrphaned(trieNode, orphaned));
    } else {
        const bool isGatewayToUsd = Ufe::SceneSegmentHandler::isGateway(ufePath);
        for (const auto& c : trieNode->childrenComponents()) {
            auto childTrieNode = (*trieNode)[c];
            if (childTrieNode) {
                // When not crossing runtimes, we can simply use the UFE path
                // component stored in the trie. When crossing runtimes, we
                // need to create a segment instead with the new runtime ID.
                if (!isGatewayToUsd) {
                    recursiveSwitch(childTrieNode, ufePath + c);
                } else {
                    Ufe::PathSegment childSegment(c, ufe::getUsdRunTimeId(), '/');
                    recursiveSwitch(childTrieNode, ufePath + childSegment);
                }
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
        auto usdAncestor = std::static_pointer_cast<UsdUfe::UsdSceneItem>(ancestor);
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
PulledPrims OrphanedNodesManager::deepCopy(const PulledPrims& src)
{
    PulledPrims dst;
    deepCopy(src.root(), dst.root());
    return dst;
}

/* static */
void OrphanedNodesManager::deepCopy(const PulledPrimNode::Ptr& src, const PulledPrimNode::Ptr& dst)
{
    for (const auto& c : src->childrenComponents()) {
        const auto& srcChild = (*src)[c];
        auto        dstChild = std::make_shared<PulledPrimNode>(c);
        dst->add(dstChild);
        if (srcChild->hasData()) {
            dstChild->setData(srcChild->data());
        }
        deepCopy(srcChild, dstChild);
    }
}

} // namespace MAYAUSD_NS_DEF
