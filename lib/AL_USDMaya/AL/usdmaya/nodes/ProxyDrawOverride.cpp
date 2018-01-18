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
#include "maya/M3dView.h"

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
    : MUserData(true)
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

//----------------------------------------------------------------------------------------------------------------------
ProxyDrawOverride::ProxyDrawOverride(const MObject& obj)
#if MAYA_API_VERSION >= 201700
  : MHWRender::MPxDrawOverride(obj, draw, true)
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

  RenderUserData* data = new RenderUserData;

  data->m_shape = (ProxyShape*)fn.userNode();
  data->m_objPath = objPath;

  auto engine = data->m_shape->engine();
  if(!engine)
  {
    data->m_shape->constructGLImagingEngine();
    engine = data->m_shape->engine();
    if(!engine)
      return data;
  }

  if(!data->m_shape || !data->m_shape->getRenderAttris(&data->m_params, frameContext, objPath))
  {
    return NULL;
  }

  data->m_params.showGuides = data->m_shape->displayGuidesPlug().asBool();
  data->m_params.showRender = data->m_shape->displayRenderGuidesPlug().asBool();
  data->m_rootPrim = data->m_shape->getRootPrim();
  data->m_engine = engine;

  return data;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyDrawOverride::draw(const MHWRender::MDrawContext& context, const MUserData* data)
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::draw\n");

  float clearCol[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

  const RenderUserData* ptr = (const RenderUserData*)data;
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
          for(int j = 0; j < positions.length(); ++j)
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

    ptr->m_engine->Render(ptr->m_rootPrim, ptr->m_params);

    auto view = M3dView::active3dView();
    const auto& paths1 = ptr->m_shape->selectedPaths();
    const auto& paths2 = ptr->m_shape->selectionList().paths();
    SdfPathVector combined;
    combined.reserve(paths1.size() + paths2.size());
    combined.insert(combined.end(), paths1.begin(), paths1.end());
    combined.insert(combined.end(), paths2.begin(), paths2.end());

    ptr->m_engine->SetSelected(combined);
    ptr->m_engine->SetSelectionColor(GfVec4f(1.0f, 2.0f/3.0f, 0.0f, 1.0f));

    if(combined.size())
    {
      UsdImagingGLEngine::RenderParams params = ptr->m_params;
      params.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME;
      MColor colour = M3dView::leadColor();
      params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);
      glDepthFunc(GL_LEQUAL);
      ptr->m_engine->RenderBatch(combined, params);
    }

    // HACK: Maya doesn't restore this ONE buffer binding after our override is done so we have to do it for them.
    glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboBinding);

    stateManager->setDepthStencilState(previousDepthState);
    MHWRender::MStateManager::releaseDepthStencilState(depthState);
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
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
