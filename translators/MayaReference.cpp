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
#include "MayaReference.h"

#include "maya/MSelectionList.h"
#include "maya/MGlobal.h"
#include "maya/MDGModifier.h"
#include "maya/MNodeClass.h"
#include "maya/MPlug.h"
#include "maya/MFnTransform.h"
#include "maya/MFnCamera.h"
#include "maya/MFileIO.h"
#include "maya/MItDag.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/fileio/translators/DgNodeTranslator.h"
#include "AL/usd/schemas/MayaReference.h"
#include "AL/usdmaya/DebugCodes.h"

IGNORE_USD_WARNINGS_PUSH
#include <pxr/usd/usd/attribute.h>
IGNORE_USD_WARNINGS_POP

#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cout << X << std::endl;
#else
# define Trace(X)
#endif

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(MayaReference, AL_usd_MayaReference)

//----------------------------------------------------------------------------------------------------------------------
const TfToken MayaReferenceLogic::m_namespaceName = TfToken("mayaNamespace");
const TfToken MayaReferenceLogic::m_referenceName = TfToken("mayaReference");

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::initialize()
{
  //Initialise all the class plugs
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::import(const UsdPrim& prim, MObject& parent)
{
  Trace("MayaReferenceLogic::import");
  MStatus status;
  status = m_mayaReferenceLogic.LoadMayaReference(prim, parent);

  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::tearDown(const SdfPath& prim)
{
  Trace("MayaReferenceLogic::tearDown");
  MObject mayaObject;
  MObjectHandle handle;
  context()->getTransform(prim, handle);
  mayaObject = handle.object();
  m_mayaReferenceLogic.UnloadMayaReference(mayaObject);
  return MS::kSuccess;
}
//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReference::update(const UsdPrim& prim)
{
  Trace("MayaReferenceLogic::update");
  MObjectHandle handle;
  if(!context()->getTransform(prim, handle))
  {
    MGlobal::displayError(MString("MayaReference::update unable to find the reference node for prim: ") + prim.GetPath().GetText());
  }
  return m_mayaReferenceLogic.update(prim, handle.object());
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::update(const UsdPrim& prim, MObject parent) const
{
  // Check to see if we have a valid Maya reference attribute
  UsdAttribute mayaReferenceAttribute = prim.GetAttribute(m_referenceName);
  
  SdfAssetPath mayaReferenceAssetPath;
  mayaReferenceAttribute.Get(&mayaReferenceAssetPath);
  MString mayaReferencePath(mayaReferenceAssetPath.GetResolvedPath().c_str());
  
  // Load file reference
  std::string rigNamespace;
  if(UsdAttribute rigNamespaceAttribute = prim.GetAttribute(m_namespaceName))
  {
    rigNamespaceAttribute.Get<std::string>(&rigNamespace);
  }

  MStatus status;
  MFnDependencyNode fnParent(parent, &status);
  if(status)
  {
    MPlug messagePlug = fnParent.findPlug("message", &status);
    if(status)
    {
      MPlugArray referencePlugs;
      messagePlug.connectedTo(referencePlugs, false, true);
      
      MString result, command, filepath;
      
      // Now we can unload and remove all references.
      for(uint32_t i = 0, n = referencePlugs.length(); i < n; ++i)
      {
        MObject temp = referencePlugs[i].node();
        if(temp.hasFn(MFn::kReference))
        {
          MFnDependencyNode fnReference(temp);
          command = MString("referenceQuery -f \"") + fnReference.name() + "\"";
          MGlobal::executeCommand(command, filepath);
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update referenceNode=%s prim=%s execute \"%s\"=%s\n",
                                              fnReference.absoluteName().asChar(),
                                              prim.GetPath().GetText(),
                                              command.asChar(),
                                              filepath.asChar());
          
          if(!rigNamespace.empty())
          {
            // check to see if the namespace has changed
            command = MString("file -q -ns \"") + filepath + "\"";
            MGlobal::executeCommand(command, result);
            TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s execute \"%s\"=%s\n",
                                                prim.GetPath().GetText(),
                                                command.asChar(),
                                                result.asChar());
            if(result != rigNamespace.c_str())
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
                MGlobal::displayError(MString("Failed to update reference with new namespace: ") + mayaReferencePath);
              }
            }
          }
          
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
              if(!MGlobal::executeCommand(command))
              {
                MGlobal::displayError(MString("Failed to update reference with new path: ") + mayaReferencePath);
              }
            }
            else
            {
              TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s loadReferenceByNode\n", prim.GetPath().GetText());
              MString s = MFileIO::loadReferenceByNode(temp, &status);
            }
          }
          else{
            TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::update prim=%s unloadReferenceByNode\n", prim.GetPath().GetText());
            MString s = MFileIO::unloadReferenceByNode(temp, &status);
          }
        }
      }
    }
  }
  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::LoadMayaReference(const UsdPrim& prim, MObject& parent) const
{
  Trace("MayaReferenceLogic::LoadMayaReference");
  MFnDagNode fn;
  // Check to see if we have a valid Maya reference attribute
  UsdAttribute mayaReferenceAttribute = prim.GetAttribute(m_referenceName);

  SdfAssetPath mayaReferenceAssetPath;
  mayaReferenceAttribute.Get(&mayaReferenceAssetPath);
  MString mayaReferencePath(mayaReferenceAssetPath.GetResolvedPath().c_str());

  //The resolved path is empty if the maya reference is a full path.
  if(!mayaReferencePath.length())
  {
    mayaReferencePath = mayaReferenceAssetPath.GetAssetPath().c_str();
  }

  //If the path is still empty return, there is no reference to import
  if(!mayaReferencePath.length())
  {
    return MS::kFailure;
  }

  // Load file reference
  std::string rigNamespace;
  if(UsdAttribute rigNamespaceAttribute = prim.GetAttribute(m_namespaceName))
  {
    rigNamespaceAttribute.Get<std::string>(&rigNamespace);
  }

  //Create a temp-group which will be used to import the reference contents into.
  MString rigNamespaceM(rigNamespace.c_str(), rigNamespace.size());
  MString tempNodeForReference = MString("__temp_reference_group_") + rigNamespaceM;

  // Create unloaded reference
  MStringArray createdNodes;
  MString referenceCommand = MString("file"
                                     " -reference"
                                     " -returnNewNodes"
                                     " -groupReference"
                                     " -deferReference true"
                                     " -mergeNamespacesOnClash false"
                                     " -ignoreVersion"
                                     " -options \"v=0;\"") +
                             MString(" -groupName \"") + tempNodeForReference +
                             MString("\" -namespace \"") + rigNamespaceM +
                             MString("\" \"") + mayaReferencePath + MString("\"");
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::LoadMayaReference prim=%s execute \"%s\"\n",
                                      prim.GetPath().GetText(),
                                      referenceCommand.asChar());
  MStatus status = MGlobal::executeCommand(referenceCommand, createdNodes);
  AL_MAYA_CHECK_ERROR(status, MString("failed to create maya reference: ") + referenceCommand);
  if(createdNodes.length() != 2){
    MGlobal::displayError(MString("Expected to get exactly 2 results from the reference command: ") + referenceCommand);
    return MS::kFailure;
  }
  // Retrieve created nodes
  MString refNode = createdNodes[0];
  tempNodeForReference = createdNodes[1];

  MSelectionList selectionList;
  selectionList.add(refNode);
  selectionList.add(tempNodeForReference);

  MObject referenceObject;
  MObject tempReferenceGroupObject;
  selectionList.getDependNode(0, referenceObject);
  selectionList.getDependNode(1, tempReferenceGroupObject);

  // Now load the reference to properly trigger the kAfterReferenceLoad callback
  referenceCommand = MString("file -loadReference \"") + refNode + MString("\"");
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("MayaReferenceLogic::LoadMayaReference prim=%s execute \"%s\"\n",
                                      prim.GetPath().GetText(),
                                      referenceCommand.asChar());
  status = MGlobal::executeCommand(referenceCommand);
  AL_MAYA_CHECK_ERROR(status, MString("failed to load reference: ") + referenceCommand);

  //Iterate through the group which houses all the items brought in by the reference.
  MFnDagNode tempReferenceGroupDag(tempReferenceGroupObject);
  
  MDagPath path;
  MFnDagNode parentDag(parent, &status);
  AL_MAYA_CHECK_ERROR(status, "failed to attach function set to parent transform for reference.");
  parentDag.getPath(path);

  //Loop through the children reparenting to the correct parent node
  while(tempReferenceGroupDag.childCount(&status))
  {
    MObject child = tempReferenceGroupDag.child(0, &status);
    AL_MAYA_CHECK_ERROR(status, "Failed to locate child on temp transform node");
    MDagPath p;
    MFnDagNode a(child);
    a.getPath(p);
    parentDag.addChild(child, MFnDagNode::kNextPos, false);//, MFnDagNode::kNextPos, /*keepExistingParents=*/false);
    AL_MAYA_CHECK_ERROR(status, "Failed to add child to prim path node");
  }

  // Connect parent node message to reference node associatedNode

  MFnDependencyNode refDependNode(referenceObject);

  MDGModifier dgMod;
  MPlug srcPlug = parentDag.findPlug("message");
  // This message attribute is used to connect specific nodes that may be associated with this reference (i.e. group,
  // locator, annotation). Use of this connection indicates that the associated nodes have the same lifespan as the
  // reference, and will be deleted along with the reference if it is removed.
  MPlug destArrayPlug = refDependNode.findPlug("associatedNode");

  // make sure we- resize the array before making any connections
  uint32_t len = destArrayPlug.numElements();
  destArrayPlug.setNumElements(len + 1);
  MPlug destPlug = destArrayPlug.elementByLogicalIndex(len);
  if (!srcPlug.isNull() && !destPlug.isNull())
  {
    MStatus status = dgMod.connect(srcPlug, destPlug);
    AL_MAYA_CHECK_ERROR(status, MString("failed to connect maya reference plug: ") + status.errorString());
    status = dgMod.doIt();
    AL_MAYA_CHECK_ERROR(status, MString("failed to execute connect maya reference plug: ") + status.errorString());
  }

  //Delete the temporary node
  MGlobal::deleteNode(tempReferenceGroupObject);

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus MayaReferenceLogic::UnloadMayaReference(MObject& parent) const
{
  Trace("MayaReferenceLogic::UnloadMayaReference");
  // going to create a temporary transform here
  MFnTransform fnt;
  MObject tempT = fnt.create();

  MStatus status;
  MFnDependencyNode fnParent(parent, &status);
  if(status)
  {
    MPlug messagePlug = fnParent.findPlug("message", &status);
    if(status)
    {
      MPlugArray referencePlugs;
      messagePlug.connectedTo(referencePlugs, false, true);
      {
        // One major problem when removing a reference from a scene is that it rather nastily will decide to 
        // destroy the associated node (which in this case happens to be a custom transform we've created to 
        // keep track of the prim path. 
        // To work around this problem, I need to re-parent anything under that transform under a temporary
        // node in order for me to safely remove the reference. 
        MFnTransform rigFn(parent);
        for(uint32_t i = 0; i < rigFn.childCount(); )
        {
          MObject c = rigFn.child(i);
          MFnDependencyNode fnc(c);

          // we need to skip transform nodes (we created them, not the reference, so please don't delete them!)
          if(fnc.typeId() != nodes::Transform::kTypeId)
          {
            status = fnt.addChild(c);
            AL_MAYA_CHECK_ERROR(status, "failed to add child transform to temp node");
          }
          else
          {
            ++i;
          }
        }

        MPlug newMessagePlug = fnt.findPlug("message");

        // create 2 modifiers, one to disconnect, one to connect.
        MDGModifier diconnectMod, connectMod;
        for(uint32_t j = 0; j < referencePlugs.length(); ++j)
        {
          diconnectMod.disconnect(messagePlug, referencePlugs[j]);
          connectMod.connect(newMessagePlug, referencePlugs[j]);
        }
        status = diconnectMod.doIt();
        AL_MAYA_CHECK_ERROR(status, "failed to disconnect plugs from reference node");
        status = connectMod.doIt();
        AL_MAYA_CHECK_ERROR(status, "failed to disconnect plugs to temp transform: ");
      }
      
      // Now we can unload and remove all references.
      for(uint32_t i = 0; i < referencePlugs.length(); ++i)
      {
        MObject temp = referencePlugs[i].node();
        if(temp.hasFn(MFn::kReference))
        {
          MString s = MFileIO::unloadReferenceByNode(temp, &status);
          AL_MAYA_CHECK_ERROR(status, "failed to unload maya reference: ");

          status = MGlobal::executeCommand(MString("file -rr \"") + s + "\"");
          AL_MAYA_CHECK_ERROR(status, "failed to remove maya reference: ");
        }
      }
    }
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
