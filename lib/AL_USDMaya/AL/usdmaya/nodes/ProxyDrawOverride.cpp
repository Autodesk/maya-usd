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
#include "pxr/imaging/glf/glew.h"
#include "AL/usdmaya/nodes/ProxyDrawOverride.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/DebugCodes.h"


#include "pxr/base/tf/envSetting.h"

#include "maya/MFnDagNode.h"
#include "maya/MBoundingBox.h"
#include "maya/MDrawContext.h"
#include "maya/MPoint.h"
#include "maya/MPointArray.h"
#include "maya/M3dView.h"
#include "maya/MSelectionContext.h"

namespace AL {
namespace usdmaya {
namespace nodes {
namespace {
//----------------------------------------------------------------------------------------------------------------------
/// \brief  user data struct - holds the info needed to render the scene
//----------------------------------------------------------------------------------------------------------------------
class RenderUserData
  : public MUserData
{
public:

  // Constructor to use when shape is drawn but no bounding box.
  RenderUserData()
    : MUserData(false)
    {}

  // Make sure everything gets freed!
  ~RenderUserData()
    {}

  UsdImagingGLEngine::RenderParams m_params;
  UsdPrim m_rootPrim;
  UsdImagingGLHdEngine* m_engine = 0;
  ProxyShape* m_shape = 0;
  MDagPath m_objPath;
};
}

//----------------------------------------------------------------------------------------------------------------------
MString ProxyDrawOverride::kDrawDbClassification("drawdb/geometry/AL_usdmaya");
MString ProxyDrawOverride::kDrawRegistrantId("pxrUsd");
MUint64 ProxyDrawOverride::s_lastRefreshFrameStamp = 0;

//----------------------------------------------------------------------------------------------------------------------
ProxyDrawOverride::ProxyDrawOverride(const MObject& obj)
#if MAYA_API_VERSION >= 201700
  : MHWRender::MPxDrawOverride(obj, draw, false)
#else
  : MHWRender::MPxDrawOverride(obj, draw)
#endif
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::ProxyDrawOverride\n");
}

//----------------------------------------------------------------------------------------------------------------------
ProxyDrawOverride::~ProxyDrawOverride()
{
}

//----------------------------------------------------------------------------------------------------------------------
MHWRender::MPxDrawOverride* ProxyDrawOverride::creator(const MObject& obj)
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::creator\n");
  return new ProxyDrawOverride(obj);
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyDrawOverride::isBounded(
    const MDagPath& objPath,
    const MDagPath& cameraPath) const
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::isBounded\n");
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MBoundingBox ProxyDrawOverride::boundingBox(
    const MDagPath& objPath,
    const MDagPath& cameraPath) const
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::boundingBox\n");
  ProxyShape* pShape = getShape(objPath);
  if (!pShape)
  {
    return MBoundingBox();
  }
  return pShape->boundingBox();
}

//----------------------------------------------------------------------------------------------------------------------
MUserData* ProxyDrawOverride::prepareForDraw(
    const MDagPath& objPath,
    const MDagPath& cameraPath,
    const MHWRender::MFrameContext& frameContext,
    MUserData* userData)
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::prepareForDraw\n");
  MFnDagNode fn(objPath);

  auto data = static_cast<RenderUserData*>(userData);
  auto shape = static_cast<ProxyShape*>(fn.userNode());
  if (!shape)
    return nullptr;

  auto engine = shape->engine();
  if(!engine)
  {
    shape->constructGLImagingEngine();
    engine = shape->engine();
    if(!engine)
      return nullptr;
  }

  RenderUserData* newData = nullptr;
  if (data == nullptr)
  {
    data = newData = new RenderUserData;
  }

  if(!shape->getRenderAttris(&data->m_params, frameContext, objPath))
  {
    delete newData;
    return nullptr;
  }

  data->m_objPath = objPath;
  data->m_shape = shape;
  data->m_rootPrim = shape->getRootPrim();
  data->m_engine = engine;

  return data;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyDrawOverride::draw(const MHWRender::MDrawContext& context, const MUserData* data)
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::draw\n");

  float clearCol[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

  RenderUserData* ptr = (RenderUserData*)data;
  if(ptr && ptr->m_rootPrim)
  {
    MHWRender::MStateManager* stateManager = context.getStateManager();
    MHWRender::MDepthStencilStateDesc depthDesc;

    auto depthState = MHWRender::MStateManager::acquireDepthStencilState(depthDesc);
    auto previousDepthState = stateManager->getDepthStencilState();
    stateManager->setDepthStencilState(depthState);

    float minR, maxR;
    context.getDepthRange(minR, maxR);

    MHWRender::MDrawContext::LightFilter considerAllSceneLights = MHWRender::MDrawContext::kFilteredToLightLimit;

    uint32_t numLights = context.numberOfActiveLights(considerAllSceneLights);

    GlfSimpleLightVector lights;
    lights.reserve(numLights);
    for(uint32_t i = 0; i < numLights; ++i)
    {
      MFloatPointArray positions;
      MFloatVector direction;
      float intensity;
      MColor color;
      bool hasDirection;
      bool hasPosition;
      context.getLightInformation(i, positions, direction, intensity, color, hasDirection, hasPosition, considerAllSceneLights);
      GlfSimpleLight light;
      if(hasPosition)
      {
        if(positions.length() == 1)
        {
          GfVec4f pos(positions[0].x, positions[0].y, positions[0].z, positions[0].w);
          light.SetPosition(pos);
        }
        else
        {
          MFloatPoint fp(0,0,0);
          for(uint32_t j = 0; j < positions.length(); ++j)
          {
            fp += positions[j];
          }
          float value = (1.0f / positions.length());
          fp.x*=value;fp.y*=value;fp.z*=value;
          light.SetPosition(GfVec4f(fp.x, fp.y, fp.z, 1.0f));
        }
      }

      if(hasDirection)
      {
        GfVec3f dir(direction.x, direction.y, direction.z);
        light.SetSpotDirection(dir);
      }

      MHWRender::MLightParameterInformation* lightParam = context.getLightParameterInformation(i, considerAllSceneLights);
      if(lightParam)
      {
        MStringArray paramNames;
        lightParam->parameterList(paramNames);
        for(uint32_t i = 0, n = paramNames.length(); i != n; ++i)
        {
          auto semantic = lightParam->parameterSemantic(paramNames[i]);
          switch(semantic)
          {
          case MHWRender::MLightParameterInformation::kIntensity:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
            }
            break;

          case MHWRender::MLightParameterInformation::kColor:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
              if(fa.length() == 3)
              {
                GfVec4f c(intensity * fa[0], intensity * fa[1], intensity * fa[2], 1.0f);
                light.SetDiffuse(c);
                light.SetSpecular(c);
              }
            }
            break;
          case MHWRender::MLightParameterInformation::kEmitsDiffuse:
          case MHWRender::MLightParameterInformation::kEmitsSpecular:
            {
              MIntArray ia;
              lightParam->getParameter(paramNames[i], ia);
            }
            break;
          case MHWRender::MLightParameterInformation::kDecayRate:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
              if (fa[0] == 0)
              {
                light.SetAttenuation(GfVec3f(1.0f, 0.0f, 0.0f));
              }
              else if (fa[0] == 1)
              {
                light.SetAttenuation(GfVec3f(0.0f, 1.0f, 0.0f));
              }
              else if (fa[0] == 2) {
                light.SetAttenuation(GfVec3f(0.0f, 0.0f, 1.0f));
              }
            }
            break;
          case MHWRender::MLightParameterInformation::kDropoff:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
              light.SetSpotFalloff(fa[0]);
            }
            break;
          case MHWRender::MLightParameterInformation::kCosConeAngle:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
              fa[0] = acos(fa[0]) * 57.295779506f;
              light.SetSpotCutoff(fa[0]);
            }
            break;
          case MHWRender::MLightParameterInformation::kStartShadowParameters:
          case MHWRender::MLightParameterInformation::kShadowBias:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
            }
            break;
          case MHWRender::MLightParameterInformation::kShadowMapSize:
          case MHWRender::MLightParameterInformation::kShadowViewProj:
            {
              MMatrix value;
              lightParam->getParameter(paramNames[i], value);
              GfMatrix4d m(value.matrix);
              light.SetShadowMatrix(m);
            }
            break;
          case MHWRender::MLightParameterInformation::kShadowColor:
            {
              MFloatArray fa;
              lightParam->getParameter(paramNames[i], fa);
              if(fa.length() == 3)
              {
                GfVec4f c(fa[0], fa[1], fa[2], 1.0f);
              }
            }
            break;
          case MHWRender::MLightParameterInformation::kGlobalShadowOn:
          case MHWRender::MLightParameterInformation::kShadowOn:
            {
              MIntArray ia;
              lightParam->getParameter(paramNames[i], ia);
              if(ia.length())
                light.SetHasShadow(ia[0]);
            }
            break;
          default:
            break;
          }
        }

        MStatus status;
        MDagPath lightPath = lightParam->lightPath(&status);
        if(status)
        {
          MMatrix wsm = lightPath.inclusiveMatrix();
          light.SetIsCameraSpaceLight(false);
          GfMatrix4d tm(wsm.inverse().matrix);
          light.SetTransform(tm);
        }
        else
        {
          light.SetIsCameraSpaceLight(true);
        }
        lights.push_back(light);
      }
    }

    auto getColour = [] (const MPlug& p) {
      GfVec4f col(0, 0, 0, 1.0f);
      MStatus status;
      MPlug pr = p.child(0, &status);
      if(status) col[0] = pr.asFloat();
      MPlug pg = p.child(1, &status);
      if(status) col[1] = pg.asFloat();
      MPlug pb = p.child(2, &status);
      if(status) col[2] = pb.asFloat();
      return col;
    };

    GlfSimpleMaterial material;
    material.SetAmbient(getColour(ptr->m_shape->ambientPlug()));
    material.SetDiffuse(getColour(ptr->m_shape->diffusePlug()));
    material.SetSpecular(getColour(ptr->m_shape->specularPlug()));
    material.SetEmission(getColour(ptr->m_shape->emissionPlug()));
    material.SetShininess(ptr->m_shape->shininessPlug().asFloat());

    GLint uboBinding = -1;
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 4, &uboBinding);

    ptr->m_engine->SetLightingState(lights, material, GfVec4f(0.05f));
    glDepthFunc(GL_LESS);

    int originX, originY, width, height;
    context.getViewportDimensions(originX, originY, width, height);

    ptr->m_engine->SetCameraState(
        GfMatrix4d(context.getMatrix(MHWRender::MFrameContext::kViewMtx).matrix),
        GfMatrix4d(context.getMatrix(MHWRender::MFrameContext::kProjectionMtx).matrix),
        GfVec4d(originX, originY, width, height));

    ptr->m_engine->SetRootTransform(GfMatrix4d(ptr->m_objPath.inclusiveMatrix().matrix));

    auto view = M3dView::active3dView();
    const auto& paths1 = ptr->m_shape->selectedPaths();
    const auto& paths2 = ptr->m_shape->selectionList().paths();
    SdfPathVector combined;
    combined.reserve(paths1.size() + paths2.size());
    combined.insert(combined.end(), paths1.begin(), paths1.end());
    combined.insert(combined.end(), paths2.begin(), paths2.end());

    ptr->m_engine->SetSelected(combined);
    ptr->m_engine->SetSelectionColor(GfVec4f(1.0f, 2.0f/3.0f, 0.0f, 1.0f));

    ptr->m_params.frame = ptr->m_shape->outTimePlug().asMTime().as(MTime::uiUnit());
    if(combined.size())
    {
      UsdImagingGLEngine::RenderParams params = ptr->m_params;
      params.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME;
      MColor colour = M3dView::leadColor();
      params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);
      glDepthFunc(GL_LEQUAL);
      ptr->m_engine->RenderBatch(combined, params);
    }

    ptr->m_engine->Render(ptr->m_rootPrim, ptr->m_params);
    
    // HACK: Maya doesn't restore this ONE buffer binding after our override is done so we have to do it for them.
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboBinding);

    stateManager->setDepthStencilState(previousDepthState);
    MHWRender::MStateManager::releaseDepthStencilState(depthState);

    // Check framestamp b/c we don't want to put multiple refresh commands
    // on the idle queue for a single frame-render... especially if we have
    // multiple ProxyShapes...
    if (!ptr->m_engine->IsConverged() && context.getFrameStamp() != s_lastRefreshFrameStamp)
    {
      s_lastRefreshFrameStamp = context.getFrameStamp();
      // Force another refresh of the current viewport
      MGlobal::executeCommandOnIdle("refresh -cv -f");
    }
  }
  glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape* ProxyDrawOverride::getShape(const MDagPath& objPath)
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::getShape\n");
  MObject obj = objPath.node();
  MFnDependencyNode dnNode(obj);
  if(obj.apiType() != MFn::kPluginShape)
  {
    return 0;
  }
  return static_cast<ProxyShape*>(dnNode.userNode());
}

//----------------------------------------------------------------------------------------------------------------------
class ProxyDrawOverrideSelectionHelper
{
public:

  static SdfPath path_ting(const SdfPath& a, const SdfPath& b, const int c)
  {
    m_paths.push_back(a);
    return a;
  }
  static SdfPathVector m_paths;
};
SdfPathVector ProxyDrawOverrideSelectionHelper::m_paths;


#if MAYA_API_VERSION >= 20190000
//----------------------------------------------------------------------------------------------------------------------
bool ProxyDrawOverride::userSelect(
    const MHWRender::MSelectionInfo& selectInfo,
    const MHWRender::MDrawContext& context,
    const MDagPath& objPath,
    const MUserData* data,
    MSelectionList& selectionList,
    MPointArray& worldSpaceHitPts)
{
  TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyDrawOverride::userSelect\n");
  MStatus status;

  M3dView view = M3dView::active3dView();

  // Get view matrix
  MMatrix viewMatrix = context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
  if (status != MStatus::kSuccess) return false;

  // Get projection matrix
  MMatrix projectionMatrix = context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
  if (status != MStatus::kSuccess) return false;

  // Get world to local matrix
  MMatrix invMatrix = objPath.inclusiveMatrixInverse();
  GfMatrix4d worldToLocalSpace(invMatrix.matrix);

  UsdImagingGLEngine::RenderParams params;

  GLuint glHitRecord;
  view.beginSelect(&glHitRecord, 1);
  glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix[0]);
  glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix[0]);
  view.endSelect();

  auto* proxyShape = static_cast<ProxyShape*>(getShape(objPath));
  auto engine = proxyShape->engine();
  proxyShape->m_pleaseIgnoreSelection = true;

  UsdPrim root = proxyShape->getUsdStage()->GetPseudoRoot();

  UsdImagingGLEngine::HitBatch hitBatch;
  SdfPathVector rootPath;
  rootPath.push_back(root.GetPath());

  int resolution = 10;
  MGlobal::getOptionVarValue("AL_usdmaya_selectResolution", resolution);
  if (resolution < 10) { resolution = 10; }
  if (resolution > 1024) { resolution = 1024; }


  bool hitSelected = engine->TestIntersectionBatch(
          GfMatrix4d(viewMatrix.matrix),
          GfMatrix4d(projectionMatrix.matrix),
          worldToLocalSpace,
          rootPath,
          params,
          resolution,
          ProxyDrawOverrideSelectionHelper::path_ting,
          &hitBatch);

  auto selected = false;

  auto getHitPath = [&engine] (UsdImagingGLEngine::HitBatch::const_reference& it) -> SdfPath
  {
    const UsdImagingGLEngine::HitInfo& hit = it.second;
    auto path = engine->GetPrimPathFromInstanceIndex(it.first, hit.hitInstanceIndex);
    if (!path.IsEmpty())
    {
      return path;
    }

    return it.first.StripAllVariantSelections();
  };


  auto addSelection = [&hitBatch, &selectInfo, &selectionList,
    &worldSpaceHitPts, proxyShape, &selected,
    &getHitPath] (const MString& command)
  {
    selected = true;
    MStringArray nodes;
    MGlobal::executeCommand(command, nodes, false, true);
    
    uint32_t i = 0;
    for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it, ++i)
    {
      auto path = getHitPath(*it).StripAllVariantSelections();
      auto obj = proxyShape->findRequiredPath(path);
      if (obj != MObject::kNullObj) 
      {
        MFnDagNode dagNode(obj);
        MDagPath dg;
        dagNode.getPath(dg);
        const double* p = it->second.worldSpaceHitPoint.GetArray();
        
        selectionList.add(dg);
        worldSpaceHitPts.append(MPoint(p[0], p[1], p[2]));
      }
    }
  };
  

  // Currently we have two approaches to selection. One method works with undo (but does not
  // play nicely with maya geometry). The second method doesn't work with undo, but does play
  // nicely with maya geometry.
  const int selectionMode = MGlobal::optionVarIntValue("AL_usdmaya_selectMode");
  if(1 == selectionMode)
  {
    if(hitSelected)
    {
      int mods;
      MString cmd = "getModifiers";
      MGlobal::executeCommand(cmd, mods);

      bool shiftHeld = (mods % 2);
      bool ctrlHeld = (mods / 4 % 2);
      MGlobal::ListAdjustment mode = MGlobal::kReplaceList;
      if(shiftHeld && ctrlHeld)
        mode = MGlobal::kAddToList;
      else
      if(ctrlHeld)
        mode = MGlobal::kRemoveFromList;
      else
      if(shiftHeld)
        mode = MGlobal::kXORWithList;
      
      MString command = "AL_usdmaya_ProxyShapeSelect";
      switch(mode)
      {
      case MGlobal::kReplaceList: command += " -r"; break;
      case MGlobal::kRemoveFromList: command += " -d"; break;
      case MGlobal::kXORWithList: command += " -tgl"; break;
      case MGlobal::kAddToList: command += " -a"; break;
      case MGlobal::kAddToHeadOfList: /* should never get here */ break;
      }

      for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it)
      {
        auto path = getHitPath(*it);
        command += " -pp \"";
        command += path.GetText();
        command += "\"";
      }

      MFnDependencyNode fn(proxyShape->thisMObject());
      command += " \"";
      command += fn.name();
      command += "\"";
      MGlobal::executeCommandOnIdle(command, false);
    }
    else
    {
      MString command = "AL_usdmaya_ProxyShapeSelect -cl ";
      MFnDependencyNode fn(proxyShape->thisMObject());
      command += " \"";
      command += fn.name();
      command += "\"";
      MGlobal::executeCommandOnIdle(command, false);
    }
  }
  else
  {
    int mods;
    MString cmd = "getModifiers";
    MGlobal::executeCommand(cmd, mods);
    
    bool shiftHeld = (mods % 2);
    bool ctrlHeld = (mods / 4 % 2);
    MGlobal::ListAdjustment mode = MGlobal::kReplaceList;
    if(shiftHeld && ctrlHeld)
      mode = MGlobal::kAddToList;
    else
    if(ctrlHeld)
      mode = MGlobal::kRemoveFromList;
    else
    if(shiftHeld)
      mode = MGlobal::kXORWithList;

    SdfPathVector paths;
    if (!hitBatch.empty())
    {
      paths.reserve(hitBatch.size());

      auto addHit = [&engine, &paths, &getHitPath](UsdImagingGLEngine::HitBatch::const_reference& it)
      {
        paths.push_back(getHitPath(it));
      };

      // Do to the inaccuracies in the selection method in gl engine
      // we still need to find the closest selection.
      // Around the edges it often selects two or more prims.
      if (selectInfo.singleSelection())
      {
        auto closestHit = hitBatch.cbegin();

        if (hitBatch.size() > 1)
        {
          MDagPath cameraPath;
          view.getCamera(cameraPath);
          const auto cameraPoint = cameraPath.inclusiveMatrix() * MPoint(0.0, 0.0, 0.0, 1.0);
          auto distanceToCameraSq = [&cameraPoint] (UsdImagingGLEngine::HitBatch::const_reference& it) -> double
          {
            const auto dx = cameraPoint.x - it.second.worldSpaceHitPoint[0];
            const auto dy = cameraPoint.y - it.second.worldSpaceHitPoint[1];
            const auto dz = cameraPoint.z - it.second.worldSpaceHitPoint[2];
            return dx * dx + dy * dy + dz * dz;
          };

          auto closestDistance = distanceToCameraSq(*closestHit);
          for (auto it = ++hitBatch.cbegin(), itEnd = hitBatch.cend(); it != itEnd; ++it)
          {
            const auto currentDistance = distanceToCameraSq(*it);
            if (currentDistance < closestDistance)
            {
              closestDistance = currentDistance;
              closestHit = it;
            }
          }
        }
        addHit(*closestHit);
      }
      else
      {
        for (const auto& it : hitBatch)
        {
          addHit(it);
        }
      }
    }

    switch(mode)
    {
    case MGlobal::kReplaceList:
      {
        MString command;
        if(!proxyShape->selectedPaths().empty())
        {
          command = "AL_usdmaya_ProxyShapeSelect -i -cl ";
          MFnDependencyNode fn(proxyShape->thisMObject());
          command += " \"";
          command += fn.name();
          command += "\";";
        }

        if(!paths.empty())
        {
          command += "AL_usdmaya_ProxyShapeSelect -i -a ";
          for(const auto& it : paths)
          {
            command += " -pp \"";
            command += it.GetText();
            command += "\"";
          }
          MFnDependencyNode fn(proxyShape->thisMObject());
          command += " \"";
          command += fn.name();
          command += "\"";

        }

        if(command.length() > 0)
        {
          addSelection(command);
        }
      }
      break;

    case MGlobal::kAddToHeadOfList:
    case MGlobal::kAddToList:
      {
        MString command;
        if(paths.size())
        {
          command = "AL_usdmaya_ProxyShapeSelect -i -a ";
          for(auto it : paths)
          {
            command += " -pp \"";
            command += it.GetText();
            command += "\"";
          }
          MFnDependencyNode fn(proxyShape->thisMObject());
          command += " \"";
          command += fn.name();
          command += "\"";
        }

        if(command.length() > 0)
        {
          selected = true;
          addSelection(command);
        }
      }
      break;

    case MGlobal::kRemoveFromList:
      {
        if(!proxyShape->selectedPaths().empty() && paths.size())
        {
          MString command = "AL_usdmaya_ProxyShapeSelect -d ";
          for(auto it : paths)
          {
            command += " -pp \"";
            command += it.GetText();
            command += "\"";
          }
          MFnDependencyNode fn(proxyShape->thisMObject());
          command += " \"";
          command += fn.name();
          command += "\"";
          MGlobal::executeCommandOnIdle(command, false);
          
        }
      }
      break;

    case MGlobal::kXORWithList:
      {
        auto& slpaths = proxyShape->selectedPaths();
        bool hasSelectedItems = false;
        bool hasDeletedItems = false;

        MString selectcommand = "AL_usdmaya_ProxyShapeSelect -i -a ";
        MString deselectcommand = "AL_usdmaya_ProxyShapeSelect -d ";
        for(auto it : paths)
        {
          bool flag = false;
          for(auto sit : slpaths)
          {
            if(sit == it)
            {
              flag = true;
              break;
            }
          }
          if(flag)
          {
            deselectcommand += " -pp \"";
            deselectcommand += it.GetText();
            deselectcommand += "\"";
            hasDeletedItems = true;
          }
          else
          {
            selectcommand += " -pp \"";
            selectcommand += it.GetText();
            selectcommand += "\"";
            hasSelectedItems = true;
          }
        }
        MFnDependencyNode fn(proxyShape->thisMObject());
        selectcommand += " \"";
        selectcommand += fn.name();
        selectcommand += "\"";
        deselectcommand += " \"";
        deselectcommand += fn.name();
        deselectcommand += "\"";

        if(hasSelectedItems)
        {
          addSelection(selectcommand);
        }

        if(hasDeletedItems)
        {
          MGlobal::executeCommandOnIdle(deselectcommand, false);
        }
      }
      break;
    }

    MString final_command = "AL_usdmaya_ProxyShapePostSelect \"";
    MFnDependencyNode fn(proxyShape->thisMObject());
    final_command += fn.name();
    final_command += "\"";
    proxyShape->setChangedSelectionState(true);
    MGlobal::executeCommandOnIdle(final_command, false);
  }

  ProxyDrawOverrideSelectionHelper::m_paths.clear();
  
  return selected;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
