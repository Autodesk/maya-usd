//
// Copyright 2017 Animal Logic
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
#include "MayaReference.h"

#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MDGModifier.h"
#include "maya/MNodeClass.h"
#include "maya/MPlug.h"
#include "maya/MFnStringData.h"
#include "maya/MFnTransform.h"
#include "maya/MFnTypedAttribute.h"
#include "maya/MFnTransform.h"
#include "maya/MFnCamera.h"
#include "maya/MFileIO.h"
#include "maya/MItDependencyNodes.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usdmaya/utils/DgNodeHelper.h"
#include "AL/usd/schemas/maya/MayaReference.h"
#include "AL/usdmaya/DebugCodes.h"

#include "AL/usdmaya/utils/Utils.h"
#include "AL/maya/utils/Utils.h"

#include <pxr/usd/usd/attribute.h>


namespace {
  // If the given source and destArrayPlug are already connected, returns the index they are
  // connected at; otherwise, returns the lowest index in the destArray that does not already
  // have a connection
  MStatus connectedOrFirstAvailableIndex(
      MPlug srcPlug,
      MPlug destArrayPlug,
      unsigned int& foundIndex,
      bool& wasConnected)
  {
    // Want to find the lowest unconnected (as dest) open logical index... so we add to
    // a list, then sort
    MStatus status;
    foundIndex = 0;
    wasConnected = false;
    unsigned int numConnected = destArrayPlug.numConnectedElements(&status);
    AL_MAYA_CHECK_ERROR(status, MString("failed to get numConnectedElements on ") + destArrayPlug.name());
    if (numConnected > 0)
    {
      std::vector<unsigned int> usedLogicalIndices;
      usedLogicalIndices.reserve(numConnected);
      MPlug elemPlug;
      MPlug elemSrcPlug;
      for (unsigned int connectedI = 0; connectedI < numConnected; ++connectedI)
      {
        elemPlug = destArrayPlug.connectionByPhysicalIndex(connectedI, &status);
        AL_MAYA_CHECK_ERROR(status, MString("failed to get connection ") + connectedI + " on "+ destArrayPlug.name());
        elemSrcPlug = elemPlug.source(&status);
        AL_MAYA_CHECK_ERROR(status, MString("failed to get source for ") + elemPlug.name());
        if (!elemSrcPlug.isNull())
        {
          if (elemSrcPlug == srcPlug)
          {
            foundIndex = elemPlug.logicalIndex();
            wasConnected = true;
            return status;
          }
          usedLogicalIndices.push_back(elemPlug.logicalIndex());
        }
      }
      if (!usedLogicalIndices.empty())
      {
        std::sort(usedLogicalIndices.begin(), usedLogicalIndices.end());
        // after sorting, since we assume no repeated indices, if the number of
        // elements = value of last element + 1, then we know it's tightly packed...
        if (usedLogicalIndices.size() - 1 == usedLogicalIndices.back())
        {
          foundIndex = usedLogicalIndices.size();
        }
        else
        {
          // If it's not tightly packed, just iterate through from start until we
          // find an element whose index != it's value
          for (foundIndex = 0;
              foundIndex < usedLogicalIndices.size();
              ++foundIndex)
          {
            if (usedLogicalIndices[foundIndex] != foundIndex) break;
          }
        }
      }
    }
    return status;
  }
}

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(MayaReference, AL_usd_MayaReference)

//----------------------------------------------------------------------------------------------------------------------
const TfToken MayaReferenceLogic::m_namespaceName = TfToken("mayaNamespace");
const TfToken MayaReferenceLogic::m_referenceName = TfToken("mayaReference");
const char* const MayaReferenceLogic::m_primNSAttr = "usdPrimNamespace";

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::initialize()
{
  //Initialise all the class plugs
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::import(const UsdPrim& prim, MObject& parent, MObject& createdObj)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReference::import prim=%s\n", prim.GetPath().GetText());
  return m_mayaReferenceLogic.update(prim, parent);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::tearDown(const SdfPath& primPath)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReference::tearDown prim=%s\n", primPath.GetText());
  MObject mayaObject;
  MObjectHandle handle;
  context()->getTransform(primPath, handle);
  mayaObject = handle.object();
  m_mayaReferenceLogic.UnloadMayaReference(mayaObject);
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::update(const UsdPrim& prim)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReference::update prim=%s\n", prim.GetPath().GetText());
  MObjectHandle handle;
  if(!context()->getTransform(prim, handle))
  {
    MGlobal::displayError(MString("MayaReference::update unable to find the transform node for prim: ") + prim.GetPath().GetText());
  }
  MObject parent = handle.object();
  return m_mayaReferenceLogic.update(prim, parent);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::update(const UsdPrim& prim, MObject parent) const
{
  MStatus status;
  SdfAssetPath mayaReferenceAssetPath;
  // Check to see if we have a valid Maya reference attribute
  UsdAttribute mayaReferenceAttribute = prim.GetAttribute(m_referenceName);
  mayaReferenceAttribute.Get(&mayaReferenceAssetPath);
  MString mayaReferencePath(mayaReferenceAssetPath.GetResolvedPath().c_str());

  // The resolved path is empty if the maya reference is a full path.
  if(!mayaReferencePath.length())
  {
    mayaReferencePath = mayaReferenceAssetPath.GetAssetPath().c_str();
  }

  // If the path is still empty return, there is no reference to import
  if(!mayaReferencePath.length())
  {
    return MS::kFailure;
  }

  // Get required namespace attribute from prim
  std::string rigNamespace;
  if(UsdAttribute rigNamespaceAttribute = prim.GetAttribute(m_namespaceName))
  {
    if (!rigNamespaceAttribute.Get<std::string>(&rigNamespace))
    {
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update Missing namespace on prim \"%s\". Will create one from prim path.", prim.GetPath().GetText());
      // Creating default namespace from prim path. Converts /a/b/c to a_b_c.
      rigNamespace = prim.GetPath().GetString();
      std::replace(rigNamespace.begin()+1, rigNamespace.end(), '/', '_');
    }
  }

  MString rigNamespaceM(rigNamespace.c_str(), rigNamespace.size());

  MFnDagNode parentDag(parent, &status);
  AL_MAYA_CHECK_ERROR(status, "failed to attach function set to parent transform for reference.");

  MObject refNode;

  // First, see if a reference is already attached
  MFnDependencyNode fnParent(parent, &status);
  if(status)
  {
    MPlug messagePlug = fnParent.findPlug("message", &status);
    MPlugArray referencePlugs;
    messagePlug.connectedTo(referencePlugs, false, true);
    for (uint32_t i = 0, n = referencePlugs.length(); i < n; ++i) {
      MObject temp = referencePlugs[i].node();
      if (temp.hasFn(MFn::kReference)) {
        refNode = temp;
      }
    }
  }

  // Check to see if we already have a reference matching the prim's namespace.
  if(refNode.isNull())
  {
    for (MItDependencyNodes refIter(MFn::kReference); !refIter.isDone(); refIter.next()) {
      MObject tempRefNode = refIter.item();
      MFnReference tempRefFn(tempRefNode);
      if (!tempRefFn.isFromReferencedFile()) {
        MPlug primNSPlug = tempRefFn.findPlug(MString(m_primNSAttr), true, &status);
        if (status == MS::kInvalidParameter) {
          // No prim NS attribute. These aren't the droids we're looking for.
          continue;
        }

        // Test the namespace
        if (primNSPlug.asString() == rigNamespaceM) {
          // We found one with the same namespace - continue running an update

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
  if (refNode.isNull())
  {
    return LoadMayaReference(prim, parent, mayaReferencePath, rigNamespaceM);
  }

  if(status)
  {
    MString command, filepath;
    MFnReference fnReference(refNode);
    command = MString("referenceQuery -f -withoutCopyNumber \"") + fnReference.name() + "\"";
    MGlobal::executeCommand(command, filepath);
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update referenceNode=%s prim=%s execute \"%s\"=%s\n",
#if MAYA_API_VERSION < 201700
                                        fnReference.name().asChar(),
#else
                                        fnReference.absoluteName().asChar(),
#endif
                                        prim.GetPath().GetText(),
                                        command.asChar(),
                                        filepath.asChar());

    if(prim.IsActive())
    {
      if(mayaReferencePath.length() != 0 && filepath != mayaReferencePath)
      {
        command = "file -loadReference \"";
        command += fnReference.name();
        command += "\" \"";
        command += mayaReferencePath;
        command += "\"";
        TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s execute %s\n",
                                            prim.GetPath().GetText(),
                                            command.asChar());
        status = MGlobal::executeCommand(command);
        AL_MAYA_CHECK_ERROR(status, MString("Failed to update reference with new path: ") + mayaReferencePath);
      }
      else
      {
        // Check to see if reference is already loaded - if so, don't need to do anything!
        if (fnReference.isLoaded())
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s already loaded with correct path\n", prim.GetPath().GetText());
        }
        else
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s loadReferenceByNode\n", prim.GetPath().GetText());
          MString s = MFileIO::loadReferenceByNode(refNode, &status);
        }

        if(!rigNamespace.empty())
        {
          // check to see if the namespace has changed
          MString refNamespace = fnReference.associatedNamespace(true);
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s, namespace was: %s\n",
                                              prim.GetPath().GetText(),
                                              refNamespace.asChar());
          if(refNamespace != rigNamespace.c_str())
          {
            command = "file -e -ns \"";
            command += rigNamespace.c_str();
            command += "\" \"";
            command += filepath;
            command += "\"";
            TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s execute %s\n",
                                                prim.GetPath().GetText(),
                                                command.asChar());
            if(!MGlobal::executeCommand(command))
            {
              MGlobal::displayError(MString("Failed to update reference with new namespace. refNS:" + refNamespace + "rigNs: " + rigNamespace.c_str() + ": ") + mayaReferencePath);
            }
          }
        }
      }
    }
    else
    {
      // Can unconditionally unload, as unloading an already unloaded reference
      // won't do anything, and won't error
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s unloadReferenceByNode\n", prim.GetPath().GetText());
      MString s = MFileIO::unloadReferenceByNode(refNode, &status);
    }
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::LoadMayaReference(const UsdPrim& prim, MObject& parent, MString& mayaReferencePath, MString& rigNamespaceM) const
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::LoadMayaReference prim=%s\n", prim.GetPath().GetText());
  const TfToken maya_associatedReferenceNode("maya_associatedReferenceNode");
  MStatus status;

  MFnDagNode parentDag(parent, &status);
  AL_MAYA_CHECK_ERROR(status, "failed to attach function set to parent transform for reference.");

  // Need to create new reference (initially unloaded).
  MStringArray createdNodes;
  MString referenceCommand = MString("file"
                                     " -reference"
                                     " -returnNewNodes"
                                     " -deferReference true"
                                     " -mergeNamespacesOnClash false"
                                     " -ignoreVersion"
                                     " -options \"v=0;\""
                                     " -namespace \"") + rigNamespaceM +
                                     "\" \"" + mayaReferencePath + "\"";

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::LoadMayaReference prim=%s execute \"%s\"\n",
                                      prim.GetPath().GetText(),
                                      referenceCommand.asChar());
  status = MGlobal::executeCommand(referenceCommand, createdNodes);
  AL_MAYA_CHECK_ERROR(status, MString("failed to create maya reference: ") + referenceCommand);

  if (createdNodes.length() != 1)
  {
    MGlobal::displayError(MString("Expected to exactly 1 node result from reference command: ") + referenceCommand);
    return MS::kFailure;
  }

  // Retrieve created reference node
  MObject referenceObject;
  MString refNode = createdNodes[0];
  MSelectionList selectionList;
  selectionList.add(refNode);
  selectionList.getDependNode(0, referenceObject);

  // Connect prim transform's message to reference's `associatedNode` attribute,
  // so that the referenced nodes end up under the prim transform when loaded.
  MFnReference refDependNode(referenceObject);
  connectReferenceAssociatedNode(parentDag, refDependNode);

  // Now load the reference to properly trigger the kAfterReferenceLoad callback
  MFileIO::loadReferenceByNode(referenceObject, &status);
  AL_MAYA_CHECK_ERROR(status, MString("failed to load reference: ") + referenceCommand);
  {
    // To avoid the error that USD complains about editing to same layer simultaneously from different threads,
    // we record it as custom data instead of creating an attribute.
    VtValue value(AL::maya::utils::convert(refDependNode.name()));
    prim.SetCustomDataByKey(maya_associatedReferenceNode, value);
  }

  // Add attribute to the reference node to track the namespace the prim was
  // trying to use, since the actual namespace might be different.
  MObject primNSAttr = refDependNode.attribute(MString(m_primNSAttr), &status);
  if (status == MS::kInvalidParameter)
  {
    // Need to create the attribute
    MFnTypedAttribute fnAttr;
    primNSAttr = fnAttr.create(m_primNSAttr, "upns", MFnData::kString);
    // Temporarily unlock reference node (will be locked by default).
    refDependNode.setLocked(false);
    status = refDependNode.addAttribute(primNSAttr);
    refDependNode.setLocked(true);
    AL_MAYA_CHECK_ERROR(status, "failed to add usdPrimPath attr to reference node");
  }
  else if (status == MS::kFailure)
  {
    // Something very wrong. Deal with this later
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("failed to query usdPrimNamespace attribute\n");
  }

  if (status == MS::kSuccess)
  {
    MDGModifier attrMod;
    status = attrMod.newPlugValueString(MPlug(referenceObject, primNSAttr), rigNamespaceM);
    AL_MAYA_CHECK_ERROR(status, "failed to set usdPrimPath attr on reference node");
    status = attrMod.doIt();
    AL_MAYA_CHECK_ERROR(status, "failed to execute reference attr modifier");
  }

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::UnloadMayaReference(MObject& parent) const
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::UnloadMayaReference\n");

  MStatus status;
  MFnDependencyNode fnParent(parent, &status);
  if(status)
  {
    MPlug messagePlug = fnParent.findPlug("message", &status);
    if(status)
    {
      MPlugArray referencePlugs;
      messagePlug.connectedTo(referencePlugs, false, true);
      
      // Unload the connected references.
      for(uint32_t i = 0; i < referencePlugs.length(); ++i)
      {
        MObject temp = referencePlugs[i].node();
        if(temp.hasFn(MFn::kReference))
        {
          MFileIO::unloadReferenceByNode(temp, &status);
          AL_MAYA_CHECK_ERROR(status, "failed to unload maya reference");
        }
      }
    }
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::connectReferenceAssociatedNode(MFnDagNode& dagNode, MFnReference& refNode) const
{
  MPlug srcPlug = dagNode.findPlug("message");
  /*
     From the Maya docs:
     > This message attribute is used to connect specific nodes that may be
     > associated with this reference (i.e. group, locator, annotation). Use of
     > this connection indicates that the associated nodes have the same
     > lifespan as the reference, and will be deleted along with the reference
     > if it is removed.
   */
  MStatus result;
  MPlug destArrayPlug = refNode.findPlug("associatedNode");
  bool wasConnected = false;
  unsigned int destIndex = 0;
  result = connectedOrFirstAvailableIndex(srcPlug, destArrayPlug, destIndex, wasConnected);
  AL_MAYA_CHECK_ERROR(result, MString("failed to calculate first available dest index for ") + destArrayPlug.name());
  if (wasConnected)
  {
    // If it's already connected, abort, we're done
    return result;
  }
  MPlug destPlug = destArrayPlug.elementByLogicalIndex(destIndex);

  result = MS::kFailure;
  if (!srcPlug.isNull() && !destPlug.isNull())
  {
    MDGModifier dgMod;
    result = dgMod.connect(srcPlug, destPlug);
    AL_MAYA_CHECK_ERROR(result, "failed to connect maya reference plug");
    result = dgMod.doIt();
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
