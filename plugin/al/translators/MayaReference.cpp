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
#include "AL/usdmaya/DebugCodes.h"

#include "AL/usdmaya/utils/Utils.h"

#include <mayaUsd/schemas/MayaReference.h>
#include <mayaUsd/fileio/translators/translatorMayaReference.h>

#include <pxr/usd/usd/attribute.h>


//namespace {
//  // If the given source and destArrayPlug are already connected, returns the index they are
//  // connected at; otherwise, returns the lowest index in the destArray that does not already
//  // have a connection
//  MStatus connectedOrFirstAvailableIndex(
//      MPlug srcPlug,
//      MPlug destArrayPlug,
//      unsigned int& foundIndex,
//      bool& wasConnected)
//  {
//    // Want to find the lowest unconnected (as dest) open logical index... so we add to
//    // a list, then sort
//    MStatus status;
//    foundIndex = 0;
//    wasConnected = false;
//    unsigned int numConnected = destArrayPlug.numConnectedElements(&status);
//    AL_MAYA_CHECK_ERROR(status, MString("failed to get numConnectedElements on ") + destArrayPlug.name());
//    if (numConnected > 0)
//    {
//      std::vector<unsigned int> usedLogicalIndices;
//      usedLogicalIndices.reserve(numConnected);
//      MPlug elemPlug;
//      MPlug elemSrcPlug;
//      for (unsigned int connectedI = 0; connectedI < numConnected; ++connectedI)
//      {
//        elemPlug = destArrayPlug.connectionByPhysicalIndex(connectedI, &status);
//        AL_MAYA_CHECK_ERROR(status, MString("failed to get connection ") + connectedI + " on "+ destArrayPlug.name());
//        elemSrcPlug = elemPlug.source(&status);
//        AL_MAYA_CHECK_ERROR(status, MString("failed to get source for ") + elemPlug.name());
//        if (!elemSrcPlug.isNull())
//        {
//          if (elemSrcPlug == srcPlug)
//          {
//            foundIndex = elemPlug.logicalIndex();
//            wasConnected = true;
//            return status;
//          }
//          usedLogicalIndices.push_back(elemPlug.logicalIndex());
//        }
//      }
//      if (!usedLogicalIndices.empty())
//      {
//        std::sort(usedLogicalIndices.begin(), usedLogicalIndices.end());
//        // after sorting, since we assume no repeated indices, if the number of
//        // elements = value of last element + 1, then we know it's tightly packed...
//        if (usedLogicalIndices.size() - 1 == usedLogicalIndices.back())
//        {
//          foundIndex = usedLogicalIndices.size();
//        }
//        else
//        {
//          // If it's not tightly packed, just iterate through from start until we
//          // find an element whose index != it's value
//          for (foundIndex = 0;
//              foundIndex < usedLogicalIndices.size();
//              ++foundIndex)
//          {
//            if (usedLogicalIndices[foundIndex] != foundIndex) break;
//          }
//        }
//      }
//    }
//    return status;
//  }
//}

namespace AL {
namespace usdmaya {
namespace fileio {
namespace translators {

AL_USDMAYA_DEFINE_TRANSLATOR(MayaReference, AL_usd_MayaReference)

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
  MStatus status;
  status = UsdMayaTranslatorMayaReference::LoadMayaReference(prim, parent);

  return status;
}

//----------------------------------------------------------------------------------------------------------------------
} // translators
} // fileio
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
