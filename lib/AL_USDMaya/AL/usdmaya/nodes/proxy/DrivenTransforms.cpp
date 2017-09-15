#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/nodes/proxy/DrivenTransforms.h"
#include "AL/usdmaya/DebugCodes.h"

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransforms::resizeDrivenTransforms(const size_t primPathCount)
{
  m_drivenPrimPaths.resize(primPathCount);
  m_drivenMatrix.resize(primPathCount, MMatrix::identity);
  m_drivenVisibility.resize(primPathCount, true);
}

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransforms::updateDrivenTransforms(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime)
{
  for (uint32_t i = 0, cnt = m_dirtyMatrices.size(); i < cnt; ++i)
  {
    int32_t idx = m_dirtyMatrices[i];
    // [RB] This seems redundant? Why not just prevent invalid data from entering the structure?
    if (idx >= drivenPrims.size())
    {
      continue;
    }
    UsdPrim& usdPrim = drivenPrims[idx];
    if (!usdPrim.IsValid())
    {
      continue;
    }
    UsdGeomXform xform(usdPrim);
    bool resetsXformStack = false;
    std::vector<UsdGeomXformOp> xformops = xform.GetOrderedXformOps(&resetsXformStack);
    bool pushed = false;
    for (auto& it : xformops)
    {
      if (it.GetOpType() == UsdGeomXformOp::TypeTransform)
      {
        nodes::TransformationMatrix::pushMatrix(m_drivenMatrix[idx], it, currentTime.as(MTime::uiUnit()));
        pushed = true;
        break;
      }
    }
    if (!pushed)
    {
      UsdGeomXformOp xformop = xform.AddTransformOp();
      nodes::TransformationMatrix::pushMatrix(m_drivenMatrix[idx], xformop, currentTime.as(MTime::uiUnit()));
    }

    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::updateDrivenTransforms %d %d %d %d  %d %d %d %d  %d %d %d %d  %d %d %d %d\n",
        m_drivenMatrix[idx][0][0],
        m_drivenMatrix[idx][0][1],
        m_drivenMatrix[idx][0][2],
        m_drivenMatrix[idx][0][3],
        m_drivenMatrix[idx][1][0],
        m_drivenMatrix[idx][1][1],
        m_drivenMatrix[idx][1][2],
        m_drivenMatrix[idx][1][3],
        m_drivenMatrix[idx][2][0],
        m_drivenMatrix[idx][2][1],
        m_drivenMatrix[idx][2][2],
        m_drivenMatrix[idx][2][3],
        m_drivenMatrix[idx][3][0],
        m_drivenMatrix[idx][3][1],
        m_drivenMatrix[idx][3][2],
        m_drivenMatrix[idx][3][3]);
  }
  m_dirtyMatrices.clear();
}

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransforms::updateDrivenVisibility(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime)
{
  for (uint32_t i = 0, cnt = m_dirtyVisibilities.size(); i < cnt; ++i)
  {
    int32_t idx = m_dirtyVisibilities[i];
    // [RB] This seems redundant? Why not just prevent invalid data from entering the structure?
    if (idx >= drivenPrims.size())
    {
      continue;
    }
    UsdPrim& usdPrim = drivenPrims[idx];
    if (!usdPrim)
    {
      continue;
    }
    UsdGeomXform xform(usdPrim);
    UsdAttribute attr = xform.GetVisibilityAttr();
    if(!attr)
    {
      attr = xform.CreateVisibilityAttr();
    }
    attr.Set(m_drivenVisibility[idx] ? UsdGeomTokens->inherited : UsdGeomTokens->invisible, currentTime.as(MTime::uiUnit()));
  }
  m_dirtyVisibilities.clear();
}

//----------------------------------------------------------------------------------------------------------------------
bool DrivenTransforms::update(UsdStageRefPtr stage, const MTime& currentTime)
{
  std::vector<UsdPrim> drivenPrims;
  drivenPrims.resize(transformCount());
  bool result = true;
  uint32_t cnt = m_drivenPrimPaths.size();
  for (uint32_t idx = 0; idx < cnt; ++idx)
  {
    auto path = m_drivenPrimPaths[idx];
    drivenPrims[idx] = stage->GetPrimAtPath(path);
    if (!drivenPrims[idx].IsValid())
    {
      MString warningMsg;
      warningMsg.format("Driven Prim [^1s] is not valid.", MString("") + idx);
      MGlobal::displayWarning(warningMsg);
      result = false;
    }
  }

  if (!dirtyMatrices().empty())
  {
    updateDrivenTransforms(drivenPrims, currentTime);
  }
  if (!dirtyVisibilities().empty())
  {
    updateDrivenVisibility(drivenPrims, currentTime);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
} // proxy
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

