#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/nodes/proxy/DrivenTransforms.h"
#include "AL/usdmaya/DebugCodes.h"

namespace AL {
namespace usdmaya {
namespace nodes {
namespace proxy {

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransforms::initTransform(uint32_t index)
{
  uint32_t newSZ = index + 1;
  m_drivenPrimPaths.resize(newSZ);
  m_drivenMatrix.resize(newSZ, MMatrix::identity);
  m_drivenVisibility.resize(newSZ, true);
}

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransforms::updateDrivenPrimPaths(
    uint32_t drivenIndex,
    std::vector<SdfPath>& drivenPaths,
    std::vector<UsdPrim>& drivenPrims,
    UsdStageRefPtr stage)
{
  uint32_t cnt = m_drivenPrimPaths.size();
  if (drivenPaths.size() < cnt)
  {
    drivenPaths.resize(cnt);
    drivenPrims.resize(cnt);
  }
  for (uint32_t idx = 0; idx < cnt; ++idx)
  {
    drivenPaths[idx] = SdfPath(m_drivenPrimPaths[idx]);
    drivenPrims[idx] = stage->GetPrimAtPath(drivenPaths[idx]);
    if (!drivenPrims[idx].IsValid())
    {
      MString warningMsg;
      warningMsg.format("Driven Prim [^1s] at Host [^2s] is not valid.", MString("") + idx, MString("") + drivenIndex);
      MGlobal::displayWarning(warningMsg);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void DrivenTransforms::updateDrivenTransforms(std::vector<UsdPrim>& drivenPrims, const MTime& currentTime)
{
  for (uint32_t i = 0, cnt = m_dirtyMatrices.size(); i < cnt; ++i)
  {
    int32_t idx = m_dirtyMatrices[i];
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
    attr.Set(m_drivenVisibility[idx] ? UsdGeomTokens->inherited : UsdGeomTokens->invisible, currentTime.as(MTime::uiUnit()));
  }
  m_dirtyVisibilities.clear();
}

//----------------------------------------------------------------------------------------------------------------------
} // proxy
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------

