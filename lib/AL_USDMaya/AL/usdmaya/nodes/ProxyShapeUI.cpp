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
#include <QGuiApplication>

#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/ProxyShapeUI.h"
#include "AL/usdmaya/nodes/ProxyDrawOverride.h"

#include "maya/M3dView.h"
#include "maya/MBoundingBox.h"
#include "maya/MMaterial.h"
#include "maya/MDagModifier.h"
#include "maya/MDGModifier.h"
#include "maya/MDrawRequest.h"
#include "maya/MFnDagNode.h"
#include "maya/MSelectionList.h"
#include "maya/MPoint.h"
#include "maya/MObjectArray.h"
#include "maya/MPointArray.h"

// printf debugging
#if 0 || AL_ENABLE_TRACE
# define Trace(X) std::cerr << X << std::endl;
#else
# define Trace(X)
#endif

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
ProxyShapeUI::ProxyShapeUI()
{
  Trace("ProxyShapeUI::ProxyShapeUI");
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShapeUI::~ProxyShapeUI()
{
  Trace("ProxyShapeUI::~ProxyShapeUI");
}

//----------------------------------------------------------------------------------------------------------------------
void* ProxyShapeUI::creator()
{
  return new ProxyShapeUI;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapeUI::getDrawRequests(const MDrawInfo& drawInfo, bool isObjectAndActiveOnly, MDrawRequestQueue& requests)
{
  Trace("ProxyShapeUI::getDrawRequests");

  MDrawRequest request = drawInfo.getPrototype(*this);

  ProxyShape* shape = static_cast<ProxyShape*>(surfaceShape());
  UsdImagingGLHdEngine* engine = shape->engine();
  if(!engine)
  {
    shape->constructGLImagingEngine();
  }

  // add the request to the queue
  requests.add(request);
}

// UsdImagingGL doesn't seem to like VP1 all that much, unless it sets the values directly from the OpenGL state.
#define USE_GL_LIGHTING_STATE 1

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapeUI::draw(const MDrawRequest& request, M3dView& view) const
{
  Trace("ProxyShapeUI::draw");

  //
  view.beginGL();

  // clear colour is not restored by hydra
  float clearCol[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

  ProxyShape* shape = static_cast<ProxyShape*>(surfaceShape());
  UsdImagingGLHdEngine* engine = shape->engine();
  if(!engine)
  {
    return;
  }

  auto stage = shape->getUsdStage();
  UsdImagingGLEngine::RenderParams params;

  params.showGuides = shape->displayGuidesPlug().asBool();
  params.showRender = shape->displayRenderGuidesPlug().asBool();

  params.frame = UsdTimeCode(shape->outTimePlug().asMTime().as(MTime::uiUnit()));
  params.complexity = 1.0f;

  MMatrix viewMatrix, projection, model;
  view.projectionMatrix(projection);
  view.modelViewMatrix(viewMatrix);
  model = request.multiPath().inclusiveMatrix();
  MMatrix invViewMatrix = viewMatrix.inverse();
  engine->SetRootTransform(GfMatrix4d(model.matrix));

  unsigned int x, y, w, h;
  view.viewport(x, y, w, h);
  engine->SetCameraState(
      GfMatrix4d((model.inverse() * viewMatrix).matrix),
      GfMatrix4d(projection.matrix),
      GfVec4d(x, y, w, h));

  switch(request.displayStyle())
  {
  case M3dView::kBoundingBox:
    params.drawMode = UsdImagingGLEngine::DRAW_POINTS;
    break;

  case M3dView::kFlatShaded:
    params.drawMode = UsdImagingGLEngine::DRAW_SHADED_FLAT;
    break;

  case M3dView::kGouraudShaded:
    params.drawMode = UsdImagingGLEngine::DRAW_SHADED_SMOOTH;
    break;

  case M3dView::kWireFrame:
    params.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME;
    break;

  case M3dView::kPoints:
    params.drawMode = UsdImagingGLEngine::DRAW_POINTS;
    break;
  }

  if(request.displayCulling())
  {
    if(!request.displayCullOpposite())
    {
      params.cullStyle = UsdImagingGLEngine::CULL_STYLE_BACK;
    }
    else
    {
      params.cullStyle = UsdImagingGLEngine::CULL_STYLE_FRONT;
    }
  }
  else
  {
    params.cullStyle = UsdImagingGLEngine::CULL_STYLE_NOTHING;
  }

  #if !USE_GL_LIGHTING_STATE
  MColor color = request.color();
  params.wireframeColor = GfVec4f(color.r, color.g, color.b, 1.0f);

  auto asMColor = [] (const MPlug& plug) {
    MColor c;
    c.r = plug.child(0).asFloat();
    c.g = plug.child(1).asFloat();
    c.b = plug.child(2).asFloat();
    return c;
  };

  const MColor amb = asMColor(shape->ambientPlug());
  const MColor dif = asMColor(shape->diffusePlug());
  const MColor spc = asMColor(shape->specularPlug());
  const MColor emi = asMColor(shape->emissionPlug());
  const float shine = shape->shininessPlug().asFloat();

  GlfSimpleMaterial usdmaterial;
  usdmaterial.SetAmbient(GfVec4f(amb.r, amb.g, amb.b, 1.0f));
  usdmaterial.SetDiffuse(GfVec4f(dif.r, dif.g, dif.b, 1.0f));
  usdmaterial.SetSpecular(GfVec4f(spc.r, spc.g, spc.b, 1.0f));
  usdmaterial.SetEmission(GfVec4f(emi.r, emi.g, emi.b, 1.0f));
  usdmaterial.SetShininess(shine);

  GLint nLights = 0;
  glGetIntegerv(GL_MAX_LIGHTS, &nLights);
  nLights += GL_LIGHT0;

  GlfSimpleLightVector lights;
  lights.reserve(nLights);

  GlfSimpleLight light;
  for(int i = GL_LIGHT0; i < nLights; ++i)
  {
    if (glIsEnabled(i))
    {
      GLfloat position[4], color[4];
      glGetLightfv(i, GL_POSITION, position);
      MPoint temp = MPoint(position) * invViewMatrix;
      light.SetPosition(GfVec4f(temp.x, temp.y, temp.z, 1.0f));

      glGetLightfv(i, GL_AMBIENT, color);
      light.SetAmbient(GfVec4f(color[0], color[1], color[2], 1.0f));

      glGetLightfv(i, GL_DIFFUSE, color);
      light.SetDiffuse(GfVec4f(color[0], color[1], color[2], 1.0f));

      glGetLightfv(i, GL_SPECULAR, color);
      light.SetSpecular(GfVec4f(color[0], color[1], color[2], 1.0f));

      lights.push_back(light);
    }
  }

  engine->SetLightingState(lights, usdmaterial, GfVec4f(0.05f));

  #else

  engine->SetLightingStateFromOpenGL();

  #endif

  auto paths = shape->selectedPaths();
  engine->SetSelected(paths);
  engine->SetSelectionColor(GfVec4f(1.0f, 2.0f/3.0f, 0.0f, 1.0f));
  engine->Render(shape->getRootPrim(), params);

  if(paths.size())
  {
    MColor colour = M3dView::leadColor();
    params.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME;
    params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);
    glDepthFunc(GL_LEQUAL);
    engine->RenderBatch(paths, params);
    glDepthFunc(GL_LESS);
  }

  glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);
  glPopClientAttrib();
  glPopAttrib();
  view.endGL();
}

//----------------------------------------------------------------------------------------------------------------------
class ProxyShapeSelectionHelper
{
public:

  static SdfPath path_ting(const SdfPath& a, const SdfPath& b, const int c)
  {
    m_paths.push_back(a);
    return a;
  }
  static SdfPathVector m_paths;
};
SdfPathVector ProxyShapeSelectionHelper::m_paths;


//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeUI::select(MSelectInfo& selectInfo, MSelectionList& selectionList, MPointArray& worldSpaceSelectPoints) const
{
  Trace("ProxyShapeUI::select");

  float clearCol[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

  M3dView view = selectInfo.view();

  MSelectionMask objectsMask(MSelectionMask::kSelectObjectsMask);

  // selectable() takes MSelectionMask&, not const MSelectionMask.  :(.
  if(!selectInfo.selectable(objectsMask))
    return false;

  MDagPath selectPath = selectInfo.selectPath();
  MMatrix invMatrix = selectPath.inclusiveMatrixInverse();

  UsdImagingGLEngine::RenderParams params;
  MMatrix viewMatrix, projectionMatrix;
  GfMatrix4d worldToLocalSpace(invMatrix.matrix);

  GLuint glHitRecord;
  view.beginSelect(&glHitRecord, 1);
  glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix[0]);
  glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix[0]);
  view.endSelect();

  ProxyShape* proxyShape = (ProxyShape*)surfaceShape();
  auto engine = proxyShape->engine();
  proxyShape->m_pleaseIgnoreSelection = true;

  UsdPrim root = proxyShape->getUsdStage()->GetPseudoRoot();
  SdfPathVector enablePaths;
  enablePaths.push_back(root.GetPath());
  UsdImagingGLEngine::HitBatch hitBatch;

  bool hitSelected = engine->TestIntersectionBatch(
          GfMatrix4d(viewMatrix.matrix),
          GfMatrix4d(projectionMatrix.matrix),
          worldToLocalSpace,
          enablePaths,
          params,
          5,
          ProxyShapeSelectionHelper::path_ting,
          &hitBatch);

  // Currently we have two approaches to selection. One method works with undo (but does not
  // play nicely with maya geometry). The second method doesn't work with undo, but does play
  // nicely with maya geometry.
  const int selectionMode = MGlobal::optionVarIntValue("AL_usdmaya_selectMode");
  if(selectionMode)
  {
    if(hitSelected)
    {
      Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
      bool shiftHeld = modifiers.testFlag(Qt::ShiftModifier);
      bool ctrlHeld = modifiers.testFlag(Qt::ControlModifier);
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
        const UsdImagingGLEngine::HitInfo& hit = it->second;
        std::string pathStr = it->first.GetText();

        // I'm not entirely sure about this, but it would appear that the returned string here has the variant name
        // tacked onto the end?
        size_t dot_location = pathStr.find_last_of('.');
        if(dot_location != std::string::npos)
        {
          pathStr = pathStr.substr(0, dot_location);
        }

        command += " -pp \"";
        command += pathStr.c_str();
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
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
    bool shiftHeld = modifiers.testFlag(Qt::ShiftModifier);
    bool ctrlHeld = modifiers.testFlag(Qt::ControlModifier);
    MGlobal::ListAdjustment mode = MGlobal::kReplaceList;
    if(shiftHeld && ctrlHeld)
      mode = MGlobal::kAddToList;
    else
    if(ctrlHeld)
      mode = MGlobal::kRemoveFromList;
    else
    if(shiftHeld)
      mode = MGlobal::kXORWithList;

    GfVec3d worldSpacePoint;
    SdfPathVector paths;
    int count = 0;
    for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it)
    {
      const UsdImagingGLEngine::HitInfo& hit = it->second;
      std::string pathStr = it->first.GetText();

      ++count;
      worldSpacePoint += hit.worldSpaceHitPoint;

      // I'm not entirely sure about this, but it would appear that the returned string here has the variant name
      // tacked onto the end?
      size_t dot_location = pathStr.find_last_of('.');
      if(dot_location != std::string::npos)
      {
        pathStr = pathStr.substr(0, dot_location);
      }
      paths.push_back(SdfPath(pathStr));
    }

    worldSpacePoint /= double(count);

    switch(mode)
    {
    case MGlobal::kReplaceList:
      {
        MString command;
        if(proxyShape->selectedPaths().size())
        {
          command = "AL_usdmaya_ProxyShapeSelect -i -cl ";
          MFnDependencyNode fn(proxyShape->thisMObject());
          command += " \"";
          command += fn.name();
          command += "\";";
        }

        if(paths.size())
        {
          command += "AL_usdmaya_ProxyShapeSelect -i -a ";
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

        if(command.length())
        {
          MStringArray nodes;
          MGlobal::executeCommand(command, nodes, false, true);

          uint32_t i = 0;
          for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it, ++i)
          {
            MSelectionList sl;
            sl.add(nodes[i]);
            const double* d = it->second.worldSpaceHitPoint.GetArray();
            selectInfo.addSelection(sl, MPoint(it->second.worldSpaceHitPoint.GetArray()), selectionList, worldSpaceSelectPoints, objectsMask, false);
          }
        }
      }
      break;

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

        if(command.length())
        {
          MStringArray nodes;
          MGlobal::executeCommand(command, nodes, false, true);

          uint32_t i = 0;
          for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it, ++i)
          {
            MSelectionList sl;
            sl.add(nodes[i]);
            const double* d = it->second.worldSpaceHitPoint.GetArray();
            selectInfo.addSelection(sl, MPoint(it->second.worldSpaceHitPoint.GetArray()), selectionList, worldSpaceSelectPoints, objectsMask, false);
          }
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
        SdfPathVector& slpaths = proxyShape->selectedPaths();
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
          MStringArray nodes;
          MGlobal::executeCommand(selectcommand, nodes, false, true);

          uint32_t i = 0;
          for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it, ++i)
          {
            MSelectionList sl;
            sl.add(nodes[i]);
            const double* d = it->second.worldSpaceHitPoint.GetArray();
            selectInfo.addSelection(sl, MPoint(it->second.worldSpaceHitPoint.GetArray()), selectionList, worldSpaceSelectPoints, objectsMask, false);
          }
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

  ProxyShapeSelectionHelper::m_paths.clear();

  // restore clear colour
  glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);

  return false;
}


//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
