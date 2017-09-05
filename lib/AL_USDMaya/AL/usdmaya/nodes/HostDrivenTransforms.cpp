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
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/HostDrivenTransforms.h"
#include "AL/maya/Common.h"

#include "maya/MGlobal.h"
#include "maya/MFnUnitAttribute.h"
#include "maya/MFnTypedAttribute.h"
#include "maya/MEulerRotation.h"
#include "maya/MMatrix.h"
#include "maya/MFnPluginData.h"

#include <algorithm>


namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(HostDrivenTransforms, AL_USDMAYA_DRIVENTRANSFORMS, AL_usdmaya);

MObject HostDrivenTransforms::m_drivenPrimPaths = MObject::kNullObj;
MObject HostDrivenTransforms::m_drivenTranslate = MObject::kNullObj;
MObject HostDrivenTransforms::m_outDrivenTransformsData = MObject::kNullObj;
MObject HostDrivenTransforms::m_drivenScale = MObject::kNullObj;
MObject HostDrivenTransforms::m_drivenRotate = MObject::kNullObj;
MObject HostDrivenTransforms::m_drivenRotateOrder = MObject::kNullObj;
MObject HostDrivenTransforms::m_drivenVisibility = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
HostDrivenTransforms::HostDrivenTransforms()
  : MPxNode()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::HostDrivenTransforms\n");
}

//----------------------------------------------------------------------------------------------------------------------
HostDrivenTransforms::~HostDrivenTransforms()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::~HostDrivenTransforms\n");
}

//----------------------------------------------------------------------------------------------------------------------
static const char* const rotate_order_strings[] =
{
  "xyz",
  "yzx",
  "zxy",
  "xzy",
  "yxz",
  "zyx",
  0
};

//----------------------------------------------------------------------------------------------------------------------
static const int16_t rotate_order_values[] =
{
  0,
  1,
  2,
  3,
  4,
  5,
  -1
};

//----------------------------------------------------------------------------------------------------------------------
MStatus HostDrivenTransforms::initialise()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::initialise\n");
  const char* errorString = "HostDrivenTransforms::initialize";
  try
  {
    setNodeType(kTypeName);
    addFrame("Driven Transforms");
    m_outDrivenTransformsData = addDataAttr("outDrivenTransformsData", "odrvtd", DrivenTransformsData::kTypeId, kReadable | kConnectable);
    m_drivenPrimPaths = addStringAttr("drivenPrimPaths", "drvpp", kInternal | kWritable | kArray | kStorable);
    m_drivenRotate = addAngle3Attr("drivenRotate", "drvr", 0, 0, 0, kWritable | kArray | kConnectable | kKeyable);
    m_drivenRotateOrder = addEnumAttr("drivenRotateOrder", "drvro", kWritable | kArray | kConnectable | kKeyable, rotate_order_strings, rotate_order_values);
    m_drivenScale = addFloat3Attr("drivenScale", "drvs", 1.0f, 1.0f, 1.0f, kWritable | kArray | kConnectable | kKeyable);
    m_drivenTranslate = addDistance3Attr("drivenTranslate", "drvt", 0, 0, 0, kWritable | kArray | kConnectable | kKeyable);
    m_drivenVisibility = addBoolAttr("drivenVisibility", "drvv", true, kWritable | kArray | kConnectable | kKeyable);

    AL_MAYA_CHECK_ERROR(attributeAffects(m_drivenPrimPaths, m_outDrivenTransformsData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_drivenRotate, m_outDrivenTransformsData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_drivenRotateOrder, m_outDrivenTransformsData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_drivenScale, m_outDrivenTransformsData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_drivenTranslate, m_outDrivenTransformsData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_drivenVisibility, m_outDrivenTransformsData), errorString);
  }
  catch (const MStatus& status)
  {
    return status;
  }
  generateAETemplate();

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void HostDrivenTransforms::resizeDrivenTransforms(proxy::DrivenTransforms& drivenTransforms)
{
  if (drivenTransforms.transformCount() < m_primPaths.size())
  {
    drivenTransforms.resizeDrivenTransforms(m_primPaths.size());
  }
  drivenTransforms.setDrivenPrimPaths(m_primPaths);
}

//----------------------------------------------------------------------------------------------------------------------
void HostDrivenTransforms::updateMatrices(MDataBlock& dataBlock, proxy::DrivenTransforms& drivenTransforms)
{
  MIntArray rotIndices;
  MIntArray rotOrdIndices;
  MIntArray scaleIndices;
  MIntArray translateIndices;
  MIntArray primPathsIndices;

  const uint32_t rotCnt = drivenRotatePlug().getExistingArrayAttributeIndices(rotIndices);
  const uint32_t rotOrdCnt = drivenRotateOrderPlug().getExistingArrayAttributeIndices(rotOrdIndices);
  const uint32_t scaleCnt = drivenScalePlug().getExistingArrayAttributeIndices(scaleIndices);
  const uint32_t translateCnt = drivenTranslatePlug().getExistingArrayAttributeIndices(translateIndices);
  const uint32_t primCnt = drivenPrimPathsPlug().getExistingArrayAttributeIndices(primPathsIndices);

  // construct the indices of the items that have changed
  std::vector<int32_t> transformIndices;
  transformIndices.reserve(primCnt << 2);
  if(rotCnt)
  {
    int32_t* const ptr = &rotIndices[0];
    transformIndices.insert(transformIndices.end(), ptr, ptr + rotCnt);
  }
  if(rotOrdCnt)
  {
    int32_t* const ptr = &rotOrdIndices[0];
    transformIndices.insert(transformIndices.end(), ptr, ptr + rotOrdCnt);
  }
  if(scaleCnt)
  {
    int32_t* const ptr = &scaleIndices[0];
    transformIndices.insert(transformIndices.end(), ptr, ptr + scaleCnt);
  }
  if(translateCnt)
  {
    int32_t* const ptr = &translateIndices[0];
    transformIndices.insert(transformIndices.end(), ptr, ptr + translateCnt);
  }

  if (!transformIndices.empty())
  {
    // sort indices
    std::sort(transformIndices.begin(), transformIndices.end());

    // prune duplicates
    auto last = std::unique(transformIndices.begin(), transformIndices.end());

    // determine highest index
    const int32_t maxIndex = *(last - 1);
    if (drivenTransforms.transformCount() <= maxIndex)
    {
      drivenTransforms.resizeDrivenTransforms(maxIndex + 1);
    }

    MArrayDataHandle rotateArray = dataBlock.inputArrayValue(m_drivenRotate);
    MArrayDataHandle rotateOrderArray = dataBlock.inputArrayValue(m_drivenRotateOrder);
    MArrayDataHandle scaleArray = dataBlock.inputArrayValue(m_drivenScale);
    MArrayDataHandle translateArray = dataBlock.inputArrayValue(m_drivenTranslate);

    for (auto iter = transformIndices.begin(); iter != last; ++iter)
    {
      const int32_t primIndex = *iter;
      MMatrix matrix = MMatrix::identity;

      // if we have a rotation value, insert the rotation into the matrix
      if (rotateArray.jumpToElement(primIndex))
      {
        // check for potential rotation order change
        int32_t rotOrdVal = 0;
        if (rotateOrderArray.jumpToElement(primIndex))
        {
          rotOrdVal = rotateOrderArray.inputValue().asInt();
        }
        MVector rotVal = rotateArray.inputValue().asVector();
        MEulerRotation eulRot(rotVal, (MEulerRotation::RotationOrder) rotOrdVal);
        matrix = eulRot.asMatrix();
      }

      // if we have a scale value, scale the x/y/z axes of the matrix
      if (scaleArray.jumpToElement(primIndex))
      {
        MVector sclVal = scaleArray.inputValue().asFloatVector();
        double* const x = matrix[0];
        double* const y = matrix[1];
        double* const z = matrix[2];
        x[0] *= sclVal.x;
        x[1] *= sclVal.x;
        x[2] *= sclVal.x;
        y[0] *= sclVal.y;
        y[1] *= sclVal.y;
        y[2] *= sclVal.y;
        z[0] *= sclVal.z;
        z[1] *= sclVal.z;
        z[2] *= sclVal.z;
      }

      // if we have a translation, assign the translate value here
      if (translateArray.jumpToElement(primIndex))
      {
        double* const w = matrix[3];
        MVector trsVal = translateArray.inputValue().asVector();
        w[0] = trsVal.x;
        w[1] = trsVal.y;
        w[2] = trsVal.z;
      }

      // assign the matrix
      drivenTransforms.dirtyMatrix(primIndex, matrix);

      TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::updateMatrix %d %d %d %d  %d %d %d %d  %d %d %d %d  %d %d %d %d\n",
          drivenTransforms.drivenMatrices()[primIndex][0][0],
          drivenTransforms.drivenMatrices()[primIndex][0][1],
          drivenTransforms.drivenMatrices()[primIndex][0][2],
          drivenTransforms.drivenMatrices()[primIndex][0][3],
          drivenTransforms.drivenMatrices()[primIndex][1][0],
          drivenTransforms.drivenMatrices()[primIndex][1][1],
          drivenTransforms.drivenMatrices()[primIndex][1][2],
          drivenTransforms.drivenMatrices()[primIndex][1][3],
          drivenTransforms.drivenMatrices()[primIndex][2][0],
          drivenTransforms.drivenMatrices()[primIndex][2][1],
          drivenTransforms.drivenMatrices()[primIndex][2][2],
          drivenTransforms.drivenMatrices()[primIndex][2][3],
          drivenTransforms.drivenMatrices()[primIndex][3][0],
          drivenTransforms.drivenMatrices()[primIndex][3][1],
          drivenTransforms.drivenMatrices()[primIndex][3][2],
          drivenTransforms.drivenMatrices()[primIndex][3][3]);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void HostDrivenTransforms::updateVisibility(MDataBlock& dataBlock, proxy::DrivenTransforms& drivenTransforms)
{
  MIntArray visibilityIndices;
  const uint32_t visibilityCnt = drivenVisibilityPlug().getExistingArrayAttributeIndices(visibilityIndices);
  if (visibilityCnt)
  {
    const int32_t maxIndex = visibilityIndices[visibilityCnt - 1];
    if (drivenTransforms.transformCount() <= maxIndex)
    {
      drivenTransforms.resizeDrivenTransforms(maxIndex + 1);
    }

    MArrayDataHandle visibilityArray = dataBlock.inputArrayValue(m_drivenVisibility);
    for (uint32_t i = 0; i < visibilityCnt; ++i)
    {
      const int32_t primIndex = visibilityIndices[i];
      if (visibilityArray.jumpToElement(primIndex))
      {
        drivenTransforms.dirtyVisibility(primIndex, visibilityArray.inputValue().asBool());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus HostDrivenTransforms::compute(const MPlug& plug, MDataBlock& dataBlock)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::compute %s\n", plug.name().asChar());
  if (plug == m_outDrivenTransformsData)
  {
    MObject data;
    DrivenTransformsData* drvTransData = createData<DrivenTransformsData>(DrivenTransformsData::kTypeId, data);
    if (!drvTransData)
    {
      return MS::kFailure;
    }
    proxy::DrivenTransforms& drivenTransforms = drvTransData->m_drivenTransforms;
    resizeDrivenTransforms(drivenTransforms);
    updateMatrices(dataBlock, drivenTransforms);
    updateVisibility(dataBlock, drivenTransforms);
    MStatus status = outputDataValue(dataBlock, m_outDrivenTransformsData, drvTransData);
    if (!status)
    {
      return MS::kFailure;
    }
    return status;
  }
  return MPxNode::compute(plug, dataBlock);
}

//----------------------------------------------------------------------------------------------------------------------
bool HostDrivenTransforms::getInternalValueInContext(const MPlug& plug, MDataHandle& dataHandle, MDGContext& ctx)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::getInternalValueInContext %s\n", plug.name().asChar());
  if (plug.array() == m_drivenPrimPaths)
  {
    const uint32_t index = plug.logicalIndex();
    if (m_primPaths.size() <= index)
    {
      m_primPaths.resize(index + 1);
    }
    dataHandle.set(MString(m_primPaths[index].GetText()));
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool HostDrivenTransforms::setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::setInternalValueInContext %s\n", plug.name().asChar());
  if (plug.array() == m_drivenPrimPaths)
  {
    const uint32_t index = plug.logicalIndex();
    if (m_primPaths.size() <= index)
    {
      m_primPaths.resize(index + 1);
    }
    m_primPaths[index] = SdfPath(dataHandle.asString().asChar());
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
