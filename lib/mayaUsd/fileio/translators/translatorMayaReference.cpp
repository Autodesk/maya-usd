//
// Copyright 2016 Pixar
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

//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.//
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
#include "translatorMayaReference.h"

#include <mayaUsd/base/debugCodes.h>
#include <mayaUsd/undo/OpUndoItems.h>
#include <mayaUsd/undo/UsdUndoManager.h>
#include <mayaUsd/utils/util.h>

#include <maya/MDGModifier.h>
#include <maya/MFileIO.h>
#include <maya/MFnCamera.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTransform.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNodeClass.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
// If the given source and destArrayPlug are already connected, returns the index they are
// connected at; otherwise, returns the lowest index in the destArray that does not already
// have a connection
MStatus connectedOrFirstAvailableIndex(
    MPlug         srcPlug,
    MPlug         destArrayPlug,
    unsigned int& foundIndex,
    bool&         wasConnected)
{
    // Want to find the lowest unconnected (as dest) open logical index... so we add to
    // a list, then sort
    MStatus status;
    foundIndex = 0;
    wasConnected = false;
    unsigned int numConnected = destArrayPlug.numConnectedElements(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    if (numConnected > 0) {
        std::vector<unsigned int> usedLogicalIndices;
        usedLogicalIndices.reserve(numConnected);
        MPlug elemPlug;
        MPlug elemSrcPlug;
        for (unsigned int connectedI = 0; connectedI < numConnected; ++connectedI) {
            elemPlug = destArrayPlug.connectionByPhysicalIndex(connectedI, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            elemSrcPlug = elemPlug.source(&status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            if (!elemSrcPlug.isNull()) {
                if (elemSrcPlug == srcPlug) {
                    foundIndex = elemPlug.logicalIndex();
                    wasConnected = true;
                    return status;
                }
                usedLogicalIndices.push_back(elemPlug.logicalIndex());
            }
        }
        if (!usedLogicalIndices.empty()) {
            std::sort(usedLogicalIndices.begin(), usedLogicalIndices.end());
            // after sorting, since we assume no repeated indices, if the number of
            // elements = value of last element + 1, then we know it's tightly packed...
            if (usedLogicalIndices.size() - 1 == usedLogicalIndices.back()) {
                foundIndex = usedLogicalIndices.size();
            } else {
                // If it's not tightly packed, just iterate through from start until we
                // find an element whose index != it's value
                for (foundIndex = 0; foundIndex < usedLogicalIndices.size(); ++foundIndex) {
                    if (usedLogicalIndices[foundIndex] != foundIndex)
                        break;
                }
            }
        }
    }
    return status;
}

// Given a function set attached to a DAG node, create a string from the
// node's full path by replacing all the path dividers (|) by underscores
// and stripping off the first character so the name doesn't start with an
// underscore.  (We use this for creating unique names for Maya reference
// nodes created from ALUSD proxy nodes.)
//
MString refNameFromPath(const MFnDagNode& nodeFn)
{
    MString name = nodeFn.fullPathName();
    name.substitute("|", "_");
    name = MString(name.asChar() + 1);
    return name;
}

// Cache "associatedNode" attribute to avoid look-up cost. Since UsdMayaTranslatorMayaReference
// doesn't have initialization method we use method with static variable to defere the search when
// everything is initialized.
const MObject getAssociatedNodeAttr()
{
    static MObject associatedNodeAttr = MNodeClass("reference").attribute("associatedNode");
    return associatedNodeAttr;
}

// Cache "message" attribute to avoid look-up cost. Since UsdMayaTranslatorMayaReference doesn't
// have initialization method we use method with static variable to defere the search when
// everything is initialized.
const MObject getMessageAttr()
{
    static MObject messageAttr = MNodeClass("dagNode").attribute("message");
    return messageAttr;
}
} // namespace

const TfToken UsdMayaTranslatorMayaReference::m_namespaceName = TfToken("mayaNamespace");
const TfToken UsdMayaTranslatorMayaReference::m_referenceName = TfToken("mayaReference");

MStatus UsdMayaTranslatorMayaReference::LoadMayaReference(
    const UsdPrim& prim,
    MObject&       parent,
    MString&       mayaReferencePath,
    MString&       rigNamespaceM)
{
    TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
        .Msg("MayaReferenceLogic::LoadMayaReference prim=%s\n", prim.GetPath().GetText());
    const TfToken maya_associatedReferenceNode("maya_associatedReferenceNode");
    MStatus       status;

    MFnDagNode parentDag(parent, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Need to create new reference (initially unloaded).
    //
    // When we create reference nodes, we want a separate reference node to be
    // created for each proxy, even proxies that are duplicates of each other.
    // This is to ensure that edits to each copy of an asset are preserved
    // separately.  To this end, we must create a unique name for each proxy's
    // reference node.  Simply including namespace information (if any) from the
    // proxy node is not enough to guarantee uniqueness, since duplicates of a
    // proxy node will all have the same namespace.  So we also include a string
    // created from the full path to the proxy in the name of the reference
    // node.  The resulting name may be long, but it will be unique and the user
    // shouldn't be interacting directly with these nodes anyway.
    //
    // Note that the 'file' command will name the new reference node after the
    // given namespace.  However, the command doesn't seem to provide a reference
    // node name override option, so we will rename the node later.
    //
    // (Also note that a new namespace is currently created whenever there is
    // already a reference node that uses the same one.  If, in the future, we
    // find that reference-related workflows result in an overly large number
    // of namespaces being created and/or we run into problems where Maya doesn't
    // handle this number of workspaces efficiently, we might consider setting
    // -mergeNamespacesOnClash to true.)
    //
    MStringArray createdNodes;
    MString      referenceCommand = MString("file"
                                       " -reference"
                                       " -returnNewNodes"
                                       " -deferReference true"
                                       " -mergeNamespacesOnClash false"
                                       " -ignoreVersion"
                                       " -options \"v=0;\""
                                       " -namespace \"")
        + rigNamespaceM + "\" \"" + mayaReferencePath + "\"";

    TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
        .Msg(
            "MayaReferenceLogic::LoadMayaReference prim=%s execute \"%s\"\n",
            prim.GetPath().GetText(),
            referenceCommand.asChar());
    status = MGlobal::executeCommand(referenceCommand, createdNodes);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (createdNodes.length() != 1) {
        MGlobal::displayError(
            MString("Expected to exactly 1 node result from reference command: ")
            + referenceCommand);
        return MS::kFailure;
    }

    // Retrieve created reference node
    MObject        referenceObject;
    MString        refNode = createdNodes[0];
    MSelectionList selectionList;
    selectionList.add(refNode);
    selectionList.getDependNode(0, referenceObject);

    // Connect prim transform's message to reference's `associatedNode` attribute,
    // so that the referenced nodes end up under the prim transform when loaded.
    MFnReference refDependNode(referenceObject);
    connectReferenceAssociatedNode(parentDag, refDependNode);

    // Rename the reference node.  We want a unique name so that multiple copies
    // of a given prim can each have their own reference edits. We make a name
    // from the full path to the prim for which the reference is being created
    // (and append "_RN" to the end, to indicate it's a reference node, just
    // because).
    //
    MString uniqueRefNodeName = refNameFromPath(parentDag) + "_RN";
    refDependNode.setName(uniqueRefNodeName);

    // Now load the reference to properly trigger the kAfterReferenceLoad callback
    MFileIO::loadReferenceByNode(referenceObject, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    {
        // To avoid the error that USD complains about editing to same layer simultaneously from
        // different threads, we record it as custom data instead of creating an attribute.
        MString refDependNodeName = refDependNode.name();
        VtValue value(std::string(refDependNodeName.asChar(), refDependNodeName.length()));
        prim.SetCustomDataByKey(maya_associatedReferenceNode, value);
    }

    return MS::kSuccess;
}

MStatus UsdMayaTranslatorMayaReference::UnloadMayaReference(const MObject& parent)
{
    TF_DEBUG(PXRUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::UnloadMayaReference\n");
    MStatus           status;
    MFnDependencyNode fnParent(parent, &status);
    if (status) {
        MPlug messagePlug(fnParent.object(), getMessageAttr());
        if (status) {
            MPlugArray referencePlugs;
            messagePlug.connectedTo(referencePlugs, false, true);

            // Unload the connected references.
            auto referencePlugsLength = referencePlugs.length();
            for (uint32_t i = 0; i < referencePlugsLength; ++i) {
                MObject temp = referencePlugs[i].node();
                if (temp.hasFn(MFn::kReference)) {
                    MFileIO::unloadReferenceByNode(temp, &status);
                    CHECK_MSTATUS_AND_RETURN_IT(status);
                }
            }
        }
    }
    return status;
}

MStatus UsdMayaTranslatorMayaReference::connectReferenceAssociatedNode(
    MFnDagNode&   dagNode,
    MFnReference& refNode)
{
    MPlug srcPlug(dagNode.object(), getMessageAttr());
    /*
       From the Maya docs:
       > This message attribute is used to connect specific nodes that may be
       > associated with this reference (i.e. group, locator, annotation). Use of
       > this connection indicates that the associated nodes have the same
       > lifespan as the reference, and will be deleted along with the reference
       > if it is removed.
     */
    MStatus      result;
    MPlug        destArrayPlug(refNode.object(), getAssociatedNodeAttr());
    bool         wasConnected = false;
    unsigned int destIndex = 0;
    result = connectedOrFirstAvailableIndex(srcPlug, destArrayPlug, destIndex, wasConnected);
    CHECK_MSTATUS_AND_RETURN_IT(result);
    if (wasConnected) {
        // If it's already connected, abort, we're done
        return result;
    }
    MPlug destPlug = destArrayPlug.elementByLogicalIndex(destIndex);

    result = MS::kFailure;
    if (!srcPlug.isNull() && !destPlug.isNull()) {
        auto&        undoInfo = MAYAUSD_NS_DEF::UsdUndoManager::instance().getUndoInfo();
        MDGModifier& dgMod
            = MAYAUSD_NS_DEF::MDGModifierUndoItem::create("Connect reference node", undoInfo);
        result = dgMod.connect(srcPlug, destPlug);
        CHECK_MSTATUS_AND_RETURN_IT(result);
        result = dgMod.doIt();
    }
    return result;
}

MStatus UsdMayaTranslatorMayaReference::update(const UsdPrim& prim, MObject parent)
{
    MStatus      status;
    SdfAssetPath mayaReferenceAssetPath;
    // Check to see if we have a valid Maya reference attribute
    UsdAttribute mayaReferenceAttribute = prim.GetAttribute(m_referenceName);
    mayaReferenceAttribute.Get(&mayaReferenceAssetPath);
    MString mayaReferencePath(mayaReferenceAssetPath.GetResolvedPath().c_str());

    // The resolved path is empty if the maya reference is a full path.
    if (!mayaReferencePath.length()) {
        mayaReferencePath = mayaReferenceAssetPath.GetAssetPath().c_str();
    }

    // If the path is still empty return, there is no reference to import
    if (!mayaReferencePath.length()) {
        return MS::kFailure;
    }
    TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
        .Msg(
            "MayaReferenceLogic::update Looking for attribute on \"%s\".\"%s\"\n",
            prim.GetTypeName().GetText(),
            m_namespaceName.GetText());

    // Get required namespace attribute from prim
    std::string rigNamespace;
    if (UsdAttribute rigNamespaceAttribute = prim.GetAttribute(m_namespaceName)) {
        TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
            .Msg(
                "MayaReferenceLogic::update Checking namespace on prim \"%s\".\n",
                prim.GetPath().GetText());

        if (!rigNamespaceAttribute.Get<std::string>(&rigNamespace)) {
            TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                .Msg(
                    "MayaReferenceLogic::update Missing namespace on prim \"%s\". Will create one "
                    "from prim path.\n",
                    prim.GetPath().GetText());
            // Creating default namespace from prim path. Converts /a/b/c to a_b_c.
            rigNamespace = prim.GetPath().GetString();
            std::replace(rigNamespace.begin() + 1, rigNamespace.end(), '/', '_');
        }
    }

    MFnDagNode parentDag(parent, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString rigNamespaceM(rigNamespace.c_str(), rigNamespace.size());

    MObject refNode;

    // First, see if a reference is already attached
    MFnDependencyNode fnParent(parent, &status);
    if (status) {
        MPlug      messagePlug(fnParent.object(), getMessageAttr());
        MPlugArray referencePlugs;
        messagePlug.connectedTo(referencePlugs, false, true);
        for (uint32_t i = 0, n = referencePlugs.length(); i < n; ++i) {
            MObject temp = referencePlugs[i].node();
            if (temp.hasFn(MFn::kReference)) {
                refNode = temp;
            }
        }
    }

    // Check to see whether we have previously created a reference node for this
    // prim.  If so, we can just reuse it.
    //
    // The check is based on comparing the prim's full path and the name of the
    // reference node, which we had originally created from the prim's full
    // path.  (Because of this, if the name or parentage of the prim has changed
    // since we created the reference, we won't find the old reference node.
    // The old one will be left orphaned, a new one will be created, and we will
    // lose any reference edits we had made.  Ideally, this won't happen often.
    // To prevent the problem, we could store the name of the reference node in
    // an attribute on the prim node.  The reference node would never be renamed
    // by the user, since it's locked.  If the user were to rename the prim
    // node, we could update the reference node's name too, for consistency,
    // when the two nodes got reattached.  Not doing this currently, but can
    // revisit if renaming/reparenting of prims with reference nodes turns out
    // to be a common thing in the workflow.)
    //
    if (refNode.isNull()) {
        for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
            MObject      tempRefNode = refIter.item();
            MFnReference tempRefFn(tempRefNode);
            if (!tempRefFn.isFromReferencedFile()) {

                // Get the name of the reference node so we can check if this node
                // matches the prim we are processing.  Strip off the "_RN" suffix.
                //
                MString refNodeName = tempRefFn.name();
                MString refName(refNodeName.asChar(), refNodeName.numChars() - 3);

                // What reference node name is the prim expecting?
                MString expectedRefName = refNameFromPath(parentDag);

                // If found a match, reconnect the reference node's `associatedNode`
                // attr before loading it, since the previous connection may be gone.
                //
                if (refName == expectedRefName) {
                    // Reconnect the reference node's `associatedNode` attr before
                    // loading it, since the previous connection may be gone.
                    connectReferenceAssociatedNode(parentDag, tempRefFn);
                    refNode = tempRefNode;
                    break;
                }
            }
        }
    }

    // If no reference found, we'll need to create it. This may be the first time we are
    // bring in the reference or it may have been imported or removed directly in maya.
    if (refNode.isNull()) {
        return LoadMayaReference(prim, parent, mayaReferencePath, rigNamespaceM);
    }

    if (status) {
        MString      command, filepath;
        MFnReference fnReference(refNode);
        command = MString("referenceQuery -f -withoutCopyNumber \"") + fnReference.name() + "\"";
        MGlobal::executeCommand(command, filepath);
        TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
            .Msg(
                "MayaReferenceLogic::update referenceNode=%s prim=%s execute \"%s\"=%s\n",
                fnReference.absoluteName().asChar(),
                prim.GetPath().GetText(),
                command.asChar(),
                filepath.asChar());

        if (prim.IsActive()) {
            if (mayaReferencePath.length() != 0 && filepath != mayaReferencePath) {
                command = "file -loadReference \"";
                command += fnReference.name();
                command += "\" \"";
                command += mayaReferencePath;
                command += "\"";
                TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                    .Msg(
                        "MayaReferenceLogic::update prim=%s execute %s\n",
                        prim.GetPath().GetText(),
                        command.asChar());
                status = MGlobal::executeCommand(command);
                CHECK_MSTATUS_AND_RETURN_IT(status);
            } else {
                // Check to see if reference is already loaded - if so, don't need to do anything!
                if (fnReference.isLoaded()) {
                    TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                        .Msg(
                            "MayaReferenceLogic::update prim=%s already loaded with correct path\n",
                            prim.GetPath().GetText());
                } else {
                    TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                        .Msg(
                            "MayaReferenceLogic::update prim=%s loadReferenceByNode\n",
                            prim.GetPath().GetText());
                    MString s = MFileIO::loadReferenceByNode(refNode, &status);
                }

                if (!rigNamespace.empty()) {
                    // check to see if the namespace has changed
                    MString refNamespace = fnReference.associatedNamespace(true);
                    TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                        .Msg(
                            "MayaReferenceLogic::update prim=%s, namespace was: %s\n",
                            prim.GetPath().GetText(),
                            refNamespace.asChar());
                    if (refNamespace != rigNamespace.c_str()) {
                        command = "file -e -ns \"";
                        command += rigNamespace.c_str();
                        command += "\" \"";
                        command += filepath;
                        command += "\"";
                        TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                            .Msg(
                                "MayaReferenceLogic::update prim=%s execute %s\n",
                                prim.GetPath().GetText(),
                                command.asChar());
                        if (!MGlobal::executeCommand(command)) {
                            MGlobal::displayError(
                                MString(
                                    "Failed to update reference with new namespace. refNS:"
                                    + refNamespace + "rigNs: " + rigNamespace.c_str() + ": ")
                                + mayaReferencePath);
                        }
                    }
                }
            }
        } else {
            // Can unconditionally unload, as unloading an already unloaded reference
            // won't do anything, and won't error
            TF_DEBUG(PXRUSDMAYA_TRANSLATORS)
                .Msg(
                    "MayaReferenceLogic::update prim=%s unloadReferenceByNode\n",
                    prim.GetPath().GetText());
            MString s = MFileIO::unloadReferenceByNode(refNode, &status);
        }
    }
    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
