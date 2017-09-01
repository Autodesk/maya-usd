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
#include "maya/MFnPluginData.h"
#include "maya/MPxTransformationMatrix.h"

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
    m_outDrivenTransformsData = addDataAttr("outDrivenTransformsData", "odrvtd",
                                            DrivenTransformsData::kTypeId,
                                            kReadable | kConnectable);

    m_drivenPrimPaths = addStringAttr("drivenPrimPaths", "drvpp",
                                      kInternal | kWritable | kArray | kStorable);

    m_drivenRotate = addAngle3Attr("drivenRotate", "drvr", 0, 0, 0,
                                   kWritable | kArray | kConnectable | kKeyable);

    m_drivenRotateOrder = addEnumAttr("drivenRotateOrder", "drvro",
                                      kWritable | kArray | kConnectable | kKeyable,
                                      rotate_order_strings, rotate_order_values);

    m_drivenScale = addFloat3Attr("drivenScale", "drvs", 1.0f, 1.0f, 1.0f,
                                  kWritable | kArray | kConnectable | kKeyable);

    m_drivenTranslate = addDistance3Attr("drivenTranslate", "drvt", 0, 0, 0,
                                         kWritable | kArray | kConnectable | kKeyable);

    m_drivenVisibility = addBoolAttr("drivenVisibility", "drvv", true,
                                     kWritable | kArray | kConnectable | kKeyable);

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
void HostDrivenTransforms::updatePrimPaths(DrivenTransforms& drivenTransforms)
{
  if (drivenTransforms.transformCount() < m_primPaths.size())
  {
    drivenTransforms.initTransform(m_primPaths.size() - 1);
  }
  drivenTransforms.m_drivenPrimPaths = m_primPaths;
}

//----------------------------------------------------------------------------------------------------------------------
void HostDrivenTransforms::updateMatrix(MDataBlock& dataBlock, DrivenTransforms& drivenTransforms)
{
  MIntArray rotIndices;
  MIntArray rotOrdIndices;
  MIntArray scaleIndices;
  MIntArray translateIndices;
  MIntArray primPathsIndices;
  uint32_t rotCnt = drivenRotatePlug().getExistingArrayAttributeIndices(rotIndices);
  uint32_t rotOrdCnt = drivenRotateOrderPlug().getExistingArrayAttributeIndices(rotOrdIndices);
  uint32_t scaleCnt = drivenScalePlug().getExistingArrayAttributeIndices(scaleIndices);
  uint32_t translateCnt = drivenTranslatePlug().getExistingArrayAttributeIndices(translateIndices);
  uint32_t primCnt = drivenPrimPathsPlug().getExistingArrayAttributeIndices(primPathsIndices);
  std::vector<int32_t> transformIndices;
  transformIndices.reserve(primCnt);
  for (uint32_t i = 0; i < rotCnt; ++i)
  {
    transformIndices.emplace_back(rotIndices[i]);
  }
  for (uint32_t i = 0; i < rotOrdCnt; ++i)
  {
    transformIndices.emplace_back(rotOrdIndices[i]);
  }
  for (uint32_t i = 0; i < scaleCnt; ++i)
  {
    transformIndices.emplace_back(scaleIndices[i]);
  }
  for (uint32_t i = 0; i < translateCnt; ++i)
  {
    transformIndices.emplace_back(translateIndices[i]);
  }
  if (transformIndices.empty())
    return;

  std::sort(transformIndices.begin(), transformIndices.end());
  int32_t maxIndex = transformIndices[transformIndices.size() - 1];
  if (drivenTransforms.transformCount() <= maxIndex)
  {
    drivenTransforms.initTransform(maxIndex);
  }
  auto last = std::unique(transformIndices.begin(), transformIndices.end());
  drivenTransforms.m_dirtyMatrices.clear();
  drivenTransforms.m_dirtyMatrices.reserve(std::distance(transformIndices.begin(), last));

  MArrayDataHandle rotateArray = dataBlock.inputArrayValue(m_drivenRotate);
  MArrayDataHandle rotateOrderArray = dataBlock.inputArrayValue(m_drivenRotateOrder);
  MArrayDataHandle scaleArray = dataBlock.inputArrayValue(m_drivenScale);
  MArrayDataHandle translateArray = dataBlock.inputArrayValue(m_drivenTranslate);
  for (auto iter = transformIndices.begin(); iter != last; ++iter)
  {
    int32_t idx = *iter;
    MVector rotVal(0.0, 0.0, 0.0);
    int32_t rotOrdVal = 0;
    MVector sclVal(1.0, 1.0, 1.0);
    MVector trsVal(0.0, 0.0, 0.0);
    if (rotateArray.jumpToElement(idx))
    {
      rotVal = rotateArray.inputValue().asVector();
    }
    if (rotateOrderArray.jumpToElement(idx))
    {
      rotOrdVal = rotateOrderArray.inputValue().asInt();
    }
    if (scaleArray.jumpToElement(idx))
    {
      sclVal = scaleArray.inputValue().asFloatVector();
    }
    if (translateArray.jumpToElement(idx))
    {
      trsVal = translateArray.inputValue().asVector();
    }
    MEulerRotation eulRot(rotVal, (MEulerRotation::RotationOrder) rotOrdVal);
    MPxTransformationMatrix transformMatrix;
    transformMatrix.scaleTo(sclVal);
    transformMatrix.setRotateOrientation(eulRot, MSpace::kTransform, false);
    transformMatrix.translateTo(trsVal);

    drivenTransforms.m_drivenMatrix[idx] = transformMatrix.asMatrix();
    drivenTransforms.m_dirtyMatrices.emplace_back(idx);
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::updateMatrix %d %d %d %d  %d %d %d %d  %d %d %d %d  %d %d %d %d\n",
      drivenTransforms.m_drivenMatrix[idx][0][0],
      drivenTransforms.m_drivenMatrix[idx][0][1],
      drivenTransforms.m_drivenMatrix[idx][0][2],
      drivenTransforms.m_drivenMatrix[idx][0][3],
      drivenTransforms.m_drivenMatrix[idx][1][0],
      drivenTransforms.m_drivenMatrix[idx][1][1],
      drivenTransforms.m_drivenMatrix[idx][1][2],
      drivenTransforms.m_drivenMatrix[idx][1][3],
      drivenTransforms.m_drivenMatrix[idx][2][0],
      drivenTransforms.m_drivenMatrix[idx][2][1],
      drivenTransforms.m_drivenMatrix[idx][2][2],
      drivenTransforms.m_drivenMatrix[idx][2][3],
      drivenTransforms.m_drivenMatrix[idx][3][0],
      drivenTransforms.m_drivenMatrix[idx][3][1],
      drivenTransforms.m_drivenMatrix[idx][3][2],
      drivenTransforms.m_drivenMatrix[idx][3][3]);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void HostDrivenTransforms::updateVisibility(MDataBlock& dataBlock, DrivenTransforms& drivenTransforms)
{
  MIntArray visibilityIndices;
  uint32_t visibilityCnt = drivenVisibilityPlug().getExistingArrayAttributeIndices(visibilityIndices);
  if (visibilityCnt == 0)
    return;
  int32_t maxIndex = visibilityIndices[visibilityCnt - 1];
  if (drivenTransforms.transformCount() <= maxIndex)
  {
    drivenTransforms.initTransform(maxIndex);
  }
  drivenTransforms.m_drivenVisibility.clear();
  drivenTransforms.m_drivenVisibility.reserve(visibilityCnt);
  MArrayDataHandle visibilityArray = dataBlock.inputArrayValue(m_drivenVisibility);
  for (uint32_t i = 0; i < visibilityCnt; ++i)
  {
    int32_t idx = visibilityIndices[i];
    if (visibilityArray.jumpToElement(idx))
    {
      drivenTransforms.m_drivenVisibility[idx] = visibilityArray.inputValue().asBool();
      drivenTransforms.m_dirtyVisibilities.emplace_back(idx);
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
    DrivenTransforms& drivenTransforms = drvTransData->m_drivenTransforms;
    updatePrimPaths(drivenTransforms);
    updateMatrix(dataBlock, drivenTransforms);
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
    uint32_t index = plug.logicalIndex();
    if (m_primPaths.size() <= index)
    {
      m_primPaths.resize(index + 1);
    }
    dataHandle.set(MString(m_primPaths[index].c_str(), m_primPaths[index].length()));
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool HostDrivenTransforms::setInternalValueInContext(const MPlug& plug, const MDataHandle& dataHandle, MDGContext& ctx)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("HostDrivenTransforms::setInternalValueInContext %s\n", plug.name().asChar());
  if (plug.array() == m_drivenPrimPaths)
  {
    uint32_t index = plug.logicalIndex();
    if (m_primPaths.size() <= index)
    {
      m_primPaths.resize(index + 1);
    }
    m_primPaths[index] = convert(dataHandle.asString());
  }
  return false;
}

} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
