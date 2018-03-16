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
#include "maya/MTypes.h"
#if MAYA_API_VERSION < 201700
#include <QtGui/QApplication>
#else
#include <QGuiApplication>
#endif

#include "AL/usdmaya/DebugCodes.h"
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


namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
ProxyShapeUI::ProxyShapeUI()
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::ProxyShapeUI");
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShapeUI::~ProxyShapeUI()
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::~ProxyShapeUI");
}

//----------------------------------------------------------------------------------------------------------------------
void* ProxyShapeUI::creator()
{
  return new ProxyShapeUI;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapeUI::getDrawRequests(const MDrawInfo& drawInfo, bool isObjectAndActiveOnly, MDrawRequestQueue& requests)
{
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::getDrawRequests");

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
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::draw");

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

  SdfPathVector paths(shape->selectedPaths().cbegin(), shape->selectedPaths().cend());
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
  TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::select");

  float clearCol[4];
  glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

  M3dView view = selectInfo.view();

  MSelectionMask objectsMask(ProxyShape::s_selectionMaskName);

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

  auto* proxyShape = static_cast<ProxyShape*>(surfaceShape());
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
          ProxyShapeSelectionHelper::path_ting,
          &hitBatch);

  auto selected = false;

  auto removeVariantFromPath = [] (const SdfPath& path) -> SdfPath {
      std::string pathStr = path.GetText();

      // I'm not entirely sure about this, but it would appear that the returned string here has the variant name
      // tacked onto the end?
      size_t dot_location = pathStr.find_last_of('.');
      if(dot_location != std::string::npos) {
        pathStr = pathStr.substr(0, dot_location);
      }

      return SdfPath(pathStr);
  };

  auto getHitPath = [&engine, &removeVariantFromPath] (const UsdImagingGLEngine::HitBatch::const_reference& it) -> SdfPath {
      const UsdImagingGLEngine::HitInfo& hit = it.second;
      auto path = engine->GetPrimPathFromInstanceIndex(it.first, hit.hitInstanceIndex);
      if (!path.IsEmpty())
      {
        return path;
      }
      return removeVariantFromPath(it.first);
  };


  auto addSelection = [&hitBatch, &selectInfo, &selectionList,
      &worldSpaceSelectPoints, &objectsMask, &selected, proxyShape,
      &removeVariantFromPath] (const MString& command) {
      selected = true;
      MStringArray nodes;
      MGlobal::executeCommand(command, nodes, false, true);

      // If the selection is in a single selection mode, we don't know if your mesh
      // will be the actual final selection, because we can't make sure this is going to
      // be called the last. So we are returning a deferred command here, that'll run last.
      // That'll check if the mesh is still selected, and run an internal deselect command on that.
      const auto singleSelection = selectInfo.singleSelection();

      uint32_t i = 0;
      for(auto it = hitBatch.begin(), e = hitBatch.end(); it != e; ++it, ++i)
      {
        auto obj = proxyShape->findRequiredPath(removeVariantFromPath(it->first));
        if (obj != MObject::kNullObj) {
          MSelectionList sl;
          MFnDagNode dagNode(obj);
          MDagPath dg;
          dagNode.getPath(dg);
          sl.add(dg);
          const double* d = it->second.worldSpaceHitPoint.GetArray();
          selectInfo.addSelection(sl, MPoint(d[0], d[1], d[2], 1.0), selectionList, worldSpaceSelectPoints, objectsMask, false);
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
#if MAYA_API_VERSION < 201700
      Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
#else
      Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
#endif
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
#if MAYA_API_VERSION < 201700
    Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
#else
    Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
#endif
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

    SdfPathVector paths;
    if (!hitBatch.empty()) {
      paths.reserve(hitBatch.size());

      auto addHit = [&engine, &paths, &getHitPath](const UsdImagingGLEngine::HitBatch::const_reference& it) {
        paths.push_back(getHitPath(it));
      };

      // Do to the inaccuracies in the selection method in gl engine
      // we still need to find the closest selection.
      // Around the edges it often selects two or more prims.
      if (selectInfo.singleSelection()) {
        auto closestHit = hitBatch.cbegin();

        if (hitBatch.size() > 1) {
          MDagPath cameraPath;
          selectInfo.view().getCamera(cameraPath);
          const auto cameraPoint = cameraPath.inclusiveMatrix() * MPoint(0.0, 0.0, 0.0, 1.0);
          auto distanceToCameraSq = [&cameraPoint] (const UsdImagingGLEngine::HitBatch::const_reference& it) -> double {
              const auto dx = cameraPoint.x - it.second.worldSpaceHitPoint[0];
              const auto dy = cameraPoint.y - it.second.worldSpaceHitPoint[1];
              const auto dz = cameraPoint.z - it.second.worldSpaceHitPoint[2];
              return dx * dx + dy * dy + dz * dz;
          };

          auto closestDistance = distanceToCameraSq(*closestHit);
          for (auto it = ++hitBatch.cbegin(), itEnd = hitBatch.cend(); it != itEnd; ++it) {
            const auto currentDistance = distanceToCameraSq(*it);
            if (currentDistance < closestDistance) {
              closestDistance = currentDistance;
              closestHit = it;
            }
          }
        }
        addHit(*closestHit);
      } else {
        for (const auto& it : hitBatch) {
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

        if(command.length() > 0) {
          addSelection(command);
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

        if(command.length() > 0) {
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

        if(hasSelectedItems) {
          addSelection(selectcommand);
        }

        if(hasDeletedItems) {
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

  return selected;
}


//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
