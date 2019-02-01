//
// Copyright 2017 Animal Logic
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
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
#include "AL/usdmaya/nodes/ProxyShapeUI.h"

// In Maya-2019 and less, on Linux, Xlib.h will get included
//     (from GL/glx.h < MGl.h < M3dView.h < MPxSurfaceShapeUI.h)
// ...and Xlib.h "helpfully" does:
//   #define Bool int
// This messes with sdf/types.h:
//   SdfValueTypeName Bool;
// So, we #undef bool to get around.
// In Maya-2020+, MGl.h now includes GL/gl.h (not glx.h), which doesn't have
// this annoyance...
#if defined(LINUX) && MAYA_API_VERSION < 20200000 && defined(Bool)
#undef Bool
#endif

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/Engine.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <maya/MDrawInfo.h>
#include <maya/MDrawRequest.h>
#include <maya/MDrawRequestQueue.h>
#include <maya/MFnDagNode.h>
#include <maya/MMatrix.h>
#include <maya/MSelectInfo.h>
#include <maya/MTime.h>
#include <maya/MTypes.h>

#if defined(WANT_UFE_BUILD)
#include "AL/usdmaya/TypeIDs.h"
#include "ufe/globalSelection.h"
#include "ufe/hierarchyHandler.h"
#include "ufe/log.h"
#if UFE_PREVIEW_VERSION_NUM >= 2027
#include <ufe/namedSelection.h>
#endif
#include "ufe/observableSelection.h"
#include "ufe/runTimeMgr.h"
#include "ufe/sceneItem.h"

#include <pxr/base/arch/env.h>
#endif

#include <pxr/usd/kind/registry.h>
#include <pxr/usd/usd/modelAPI.h>

namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
/// \brief Retarget a prim based on the AL_USDMaya's pick mode settings. This will either return new
/// prim to select,
///        or the original prim if no retargetting occurred.
/// \param prim Attempt to retarget this prim.
/// \return The retargetted prim, or the original.
UsdPrim retargetSelectPrim(const UsdPrim& prim)
{
    switch (ProxyShape::PickMode(MGlobal::optionVarIntValue("AL_usdmaya_pickMode"))) {

    // Read up prim hierarchy and return first Model kind ancestor as the target prim
    case ProxyShape::PickMode::kModels: {
        UsdPrim tmpPrim = prim;
        while (tmpPrim.IsValid()) {
            TfToken kind;
            UsdModelAPI(tmpPrim).GetKind(&kind);
            if (KindRegistry::GetInstance().IsA(kind, KindTokens->model)) {
                return tmpPrim;
            }
            tmpPrim = tmpPrim.GetParent();
        }
    }

    case ProxyShape::PickMode::kPrims:
    case ProxyShape::PickMode::kInstances:
    default: {
        break;
    }
    }
    return prim;
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShapeUI::ProxyShapeUI() { TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::ProxyShapeUI\n"); }

//----------------------------------------------------------------------------------------------------------------------
ProxyShapeUI::~ProxyShapeUI() { TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::~ProxyShapeUI\n"); }

//----------------------------------------------------------------------------------------------------------------------
void* ProxyShapeUI::creator() { return new ProxyShapeUI; }

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapeUI::getDrawRequests(
    const MDrawInfo&   drawInfo,
    bool               isObjectAndActiveOnly,
    MDrawRequestQueue& requests)
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::getDrawRequests\n");

    MDrawRequest request = drawInfo.getPrototype(*this);

    // If there are no side effects to calling surfaceShape(), the following two
    // lines can be removed, as they are unused.  PPT, 8-Jan-2020.
    ProxyShape* shape = static_cast<ProxyShape*>(surfaceShape());
    (void)shape;

    // add the request to the queue
    requests.add(request);
}

// UsdImagingGL doesn't seem to like VP1 all that much, unless it sets the values directly from the
// OpenGL state.
#define USE_GL_LIGHTING_STATE 1

//----------------------------------------------------------------------------------------------------------------------
void ProxyShapeUI::draw(const MDrawRequest& request, M3dView& view) const
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::draw\n");

    //
    view.beginGL();

    // clear colour is not restored by hydra
    float clearCol[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

    ProxyShape* shape = static_cast<ProxyShape*>(surfaceShape());
    Engine*     engine = shape->engine();
    if (!engine) {
        return;
    }

    auto                     stage = shape->getUsdStage();
    UsdImagingGLRenderParams params;

    params.showGuides = shape->drawGuidePurposePlug().asBool();
    params.showProxy = shape->drawProxyPurposePlug().asBool();
    params.showRender = shape->drawRenderPurposePlug().asBool();

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
        GfMatrix4d((model.inverse() * viewMatrix).matrix), GfMatrix4d(projection.matrix));
    engine->SetRenderViewport(GfVec4d(x, y, w, h));

    switch (request.displayStyle()) {
    case M3dView::kBoundingBox: params.drawMode = UsdImagingGLDrawMode::DRAW_POINTS; break;

    case M3dView::kFlatShaded: params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_FLAT; break;

    case M3dView::kGouraudShaded: params.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH; break;

    case M3dView::kWireFrame: params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME; break;

    case M3dView::kPoints: params.drawMode = UsdImagingGLDrawMode::DRAW_POINTS; break;
    }

    if (request.displayCulling()) {
        if (!request.displayCullOpposite()) {
            params.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_BACK;
        } else {
            params.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_FRONT;
        }
    } else {
        params.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_NOTHING;
    }

#if !USE_GL_LIGHTING_STATE
    MColor color = request.color();
    params.wireframeColor = GfVec4f(color.r, color.g, color.b, 1.0f);

    auto asMColor = [](const MPlug& plug) {
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
    const float  shine = shape->shininessPlug().asFloat();

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
    for (int i = GL_LIGHT0; i < nLights; ++i) {
        if (glIsEnabled(i)) {
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
    auto          style = params.drawMode;
    auto          colour = params.wireframeColor;
    if (paths.size()) {
        MColor colour = M3dView::leadColor();
        params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
        params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);
        glDepthFunc(GL_LEQUAL);
        engine->RenderBatch(paths, params);
        glDepthFunc(GL_LESS);
    }

    params.drawMode = style;
    params.wireframeColor = colour;
    engine->SetSelected(paths);
    engine->SetSelectionColor(GfVec4f(1.0f, 2.0f / 3.0f, 0.0f, 1.0f));
    engine->Render(shape->getRootPrim(), params);

    glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);
    glPopClientAttrib();
    glPopAttrib();
    view.endGL();
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShapeUI::select(
    MSelectInfo&    selectInfo,
    MSelectionList& selectionList,
    MPointArray&    worldSpaceSelectPoints) const
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyShapeUI::select\n");

    if (!MGlobal::optionVarIntValue("AL_usdmaya_selectionEnabled"))
        return false;

    float clearCol[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

    M3dView view = selectInfo.view();

    MSelectionMask objectsMask(ProxyShape::s_selectionMaskName);

    // selectable() takes MSelectionMask&, not const MSelectionMask.  :(.
    if (!selectInfo.selectable(objectsMask))
        return false;

    MDagPath selectPath = selectInfo.selectPath();
    MMatrix  invMatrix = selectPath.inclusiveMatrixInverse();

    MMatrix    viewMatrix, projectionMatrix;
    GfMatrix4d worldToLocalSpace(invMatrix.matrix);

    GLuint glHitRecord;
    view.beginSelect(&glHitRecord, 1);
    glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix[0]);
    glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix[0]);
    view.endSelect();

    auto* proxyShape = static_cast<ProxyShape*>(surfaceShape());
    auto  engine = proxyShape->engine();
    if (!engine)
        return false;
    proxyShape->m_pleaseIgnoreSelection = true;

    UsdImagingGLRenderParams params;
    params.showGuides = proxyShape->drawGuidePurposePlug().asBool();
    params.showProxy = proxyShape->drawProxyPurposePlug().asBool();
    params.showRender = proxyShape->drawRenderPurposePlug().asBool();

    UsdPrim root = proxyShape->getUsdStage()->GetPseudoRoot();

    Engine::HitBatch hitBatch;
    SdfPathVector    rootPath;
    rootPath.push_back(root.GetPath());

    int resolution = 10;
    MGlobal::getOptionVarValue("AL_usdmaya_selectResolution", resolution);
    if (resolution < 10) {
        resolution = 10;
    }
    if (resolution > 1024) {
        resolution = 1024;
    }

    TfToken resolveMode = selectInfo.singleSelection() ? HdxPickTokens->resolveNearestToCamera
                                                       : HdxPickTokens->resolveUnique;

    bool hitSelected = engine->TestIntersectionBatch(
        GfMatrix4d(viewMatrix.matrix),
        GfMatrix4d(projectionMatrix.matrix),
        worldToLocalSpace,
        rootPath,
        params,
        resolveMode,
        resolution,
        &hitBatch);

    auto selected = false;

    auto addSelection = [&hitBatch,
                         &selectInfo,
                         &selectionList,
                         &worldSpaceSelectPoints,
                         &objectsMask,
                         &selected,
                         proxyShape](const MString& command) {
        selected = true;
        MStringArray nodes;
        MGlobal::executeCommand(command, nodes, false, true);

        // If the selection is in a single selection mode, we don't know if your mesh
        // will be the actual final selection, because we can't make sure this is going to
        // be called the last. So we are returning a deferred command here, that'll run last.
        // That'll check if the mesh is still selected, and run an internal deselect command on
        // that. const auto singleSelection = selectInfo.singleSelection();

        for (const auto& it : hitBatch) {

            // Retarget hit path based on pick mode policy. The retargeted prim must
            // align with the path used in the 'AL_usdmaya_ProxyShapeSelect' command.
            const SdfPath hitPath = it.first;
            const UsdPrim retargetedHitPrim
                = retargetSelectPrim(proxyShape->getUsdStage()->GetPrimAtPath(hitPath));
            const MObject obj = proxyShape->findRequiredPath(retargetedHitPrim.GetPath());

            if (obj != MObject::kNullObj) {
                MSelectionList sl;
                MFnDagNode     dagNode(obj);
                MDagPath       dg;
                dagNode.getPath(dg);
                sl.add(dg);
                const double* d = it.second.worldSpaceHitPoint.GetArray();
                selectInfo.addSelection(
                    sl,
                    MPoint(d[0], d[1], d[2], 1.0),
                    selectionList,
                    worldSpaceSelectPoints,
                    objectsMask,
                    false);
            }
        }
    };

    // Currently we have two approaches to selection. One method works with undo (but does not
    // play nicely with maya geometry). The second method doesn't work with undo, but does play
    // nicely with maya geometry.
    const int selectionMode = MGlobal::optionVarIntValue("AL_usdmaya_selectMode");
    if (1 == selectionMode) {
        if (hitSelected) {
            int     mods;
            MString cmd = "getModifiers";
            MGlobal::executeCommand(cmd, mods);

            bool                    shiftHeld = (mods % 2);
            bool                    ctrlHeld = (mods / 4 % 2);
            MGlobal::ListAdjustment mode = MGlobal::kReplaceList;
            if (shiftHeld && ctrlHeld)
                mode = MGlobal::kAddToList;
            else if (ctrlHeld)
                mode = MGlobal::kRemoveFromList;
            else if (shiftHeld)
                mode = MGlobal::kXORWithList;

            MString command = "AL_usdmaya_ProxyShapeSelect";
            switch (mode) {
            case MGlobal::kReplaceList: command += " -r"; break;
            case MGlobal::kRemoveFromList: command += " -d"; break;
            case MGlobal::kXORWithList: command += " -tgl"; break;
            case MGlobal::kAddToList: command += " -a"; break;
            case MGlobal::kAddToHeadOfList: /* should never get here */ break;
            }

            for (const auto& it : hitBatch) {
                command += " -pp \"";
                command += it.first.GetText();
                command += "\"";
            }

            MFnDagNode fn(proxyShape->thisMObject());
            command += " \"";
            command += fn.fullPathName();
            command += "\"";
            MGlobal::executeCommandOnIdle(command, false);
        } else {
            MString    command = "AL_usdmaya_ProxyShapeSelect -cl ";
            MFnDagNode fn(proxyShape->thisMObject());
            command += " \"";
            command += fn.fullPathName();
            command += "\"";
            MGlobal::executeCommandOnIdle(command, false);
        }
    } else {
        int     mods;
        MString cmd = "getModifiers";
        MGlobal::executeCommand(cmd, mods);

        bool                    shiftHeld = (mods % 2);
        bool                    ctrlHeld = (mods / 4 % 2);
        MGlobal::ListAdjustment mode = MGlobal::kReplaceList;
        if (shiftHeld && ctrlHeld)
            mode = MGlobal::kAddToList;
        else if (ctrlHeld)
            mode = MGlobal::kRemoveFromList;
        else if (shiftHeld)
            mode = MGlobal::kXORWithList;

        SdfPathVector paths;
        if (!hitBatch.empty()) {
            paths.reserve(hitBatch.size());
            for (const auto& it : hitBatch) {
                paths.push_back(it.first);
            }
        }
#if defined(WANT_UFE_BUILD)
        if (ArchHasEnv("MAYA_WANT_UFE_SELECTION")) {
            Ufe::HierarchyHandler::Ptr handler
                = Ufe::RunTimeMgr::instance().hierarchyHandler(USD_UFE_RUNTIME_ID);
            if (handler == nullptr) {
                MGlobal::displayError(
                    "USD Hierarchy handler has not been loaded - Picking is not possible");
                return false;
            }

#if UFE_PREVIEW_VERSION_NUM >= 2027 // #ifdef UFE_V2_FEATURES_AVAILABLE
            auto ufeSel = Ufe::NamedSelection::get("MayaSelectTool");
#else
            Ufe::Selection dstSelection; // Only used for kReplaceList
                                         // Get the paths
#endif
            if (paths.size()) {
                for (const auto& it : paths) {
                    // Build a path segment of the USD picked object
                    Ufe::PathSegment ps_usd(it.GetText(), USD_UFE_RUNTIME_ID, USD_UFE_SEPARATOR);

                    // Create a sceneItem
                    const Ufe::SceneItem::Ptr& si { handler->createItem(
                        proxyShape->ufePath() + ps_usd) };

#if UFE_PREVIEW_VERSION_NUM >= 2027 // #ifdef UFE_V2_FEATURES_AVAILABLE
                    ufeSel->append(si);
#else
                    auto globalSelection = Ufe::GlobalSelection::get();

                    switch (mode) {
                    case MGlobal::kReplaceList: {
                        // Add the sceneItem to dstSelection
                        dstSelection.append(si);
                    } break;
                    case MGlobal::kAddToList: {
                        // Add the sceneItem to global selection
                        globalSelection->append(si);
                    } break;
                    case MGlobal::kRemoveFromList: {
                        // Remove the sceneItem to global selection
                        globalSelection->remove(si);
                    } break;
                    case MGlobal::kXORWithList: {
                        if (!globalSelection->remove(si)) {
                            globalSelection->append(si);
                        }
                    } break;
                    case MGlobal::kAddToHeadOfList: {
                        // No such operation on UFE selection.
                        UFE_LOG("UFE does not support prepend to selection.");
                    } break;
                    }
#endif
                }

#if UFE_PREVIEW_VERSION_NUM < 2027 // #ifndef UFE_V2_FEATURES_AVAILABLE
                if (mode == MGlobal::kReplaceList) {
                    // Add to Global selection
                    Ufe::GlobalSelection::get()->replaceWith(dstSelection);
                }
#endif
            }
        } else {
#endif

            // Massage hit paths to align with pick mode policy
            for (std::size_t i = 0; i < paths.size(); ++i) {
                const SdfPath& path = paths[i];
                const UsdPrim  retargetedPrim
                    = retargetSelectPrim(proxyShape->getUsdStage()->GetPrimAtPath(path));
                if (retargetedPrim.GetPath() != path) {
                    paths[i] = retargetedPrim.GetPath();
                }
            }

            switch (mode) {
            case MGlobal::kReplaceList: {
                MString command;
                if (!proxyShape->selectedPaths().empty()) {
                    command = "AL_usdmaya_ProxyShapeSelect -i -cl ";
                    MFnDagNode fn(proxyShape->thisMObject());
                    command += " \"";
                    command += fn.fullPathName();
                    command += "\";";
                }

                if (!paths.empty()) {
                    command += "AL_usdmaya_ProxyShapeSelect -i -a ";
                    for (const auto& it : paths) {
                        command += " -pp \"";
                        command += it.GetText();
                        command += "\"";
                    }
                    MFnDagNode fn(proxyShape->thisMObject());
                    command += " \"";
                    command += fn.fullPathName();
                    command += "\"";
                }

                if (command.length() > 0) {
                    addSelection(command);
                }
            } break;

            case MGlobal::kAddToHeadOfList:
            case MGlobal::kAddToList: {
                MString command;
                if (paths.size()) {
                    command = "AL_usdmaya_ProxyShapeSelect -i -a ";
                    for (const auto& it : paths) {
                        command += " -pp \"";
                        command += it.GetText();
                        command += "\"";
                    }
                    MFnDagNode fn(proxyShape->thisMObject());
                    command += " \"";
                    command += fn.fullPathName();
                    command += "\"";
                }

                if (command.length() > 0) {
                    selected = true;
                    addSelection(command);
                }
            } break;

            case MGlobal::kRemoveFromList: {
                if (!proxyShape->selectedPaths().empty() && paths.size()) {
                    MString command = "AL_usdmaya_ProxyShapeSelect -d ";
                    for (const auto& it : paths) {
                        command += " -pp \"";
                        command += it.GetText();
                        command += "\"";
                    }
                    MFnDagNode fn(proxyShape->thisMObject());
                    command += " \"";
                    command += fn.fullPathName();
                    command += "\"";
                    MGlobal::executeCommandOnIdle(command, false);
                }
            } break;

            case MGlobal::kXORWithList: {
                auto& slpaths = proxyShape->selectedPaths();
                bool  hasSelectedItems = false;
                bool  hasDeletedItems = false;

                MString selectcommand = "AL_usdmaya_ProxyShapeSelect -i -a ";
                MString deselectcommand = "AL_usdmaya_ProxyShapeSelect -d ";
                for (const auto& it : paths) {
                    bool flag = false;
                    for (auto sit : slpaths) {
                        if (sit == it) {
                            flag = true;
                            break;
                        }
                    }
                    if (flag) {
                        deselectcommand += " -pp \"";
                        deselectcommand += it.GetText();
                        deselectcommand += "\"";
                        hasDeletedItems = true;
                    } else {
                        selectcommand += " -pp \"";
                        selectcommand += it.GetText();
                        selectcommand += "\"";
                        hasSelectedItems = true;
                    }
                }
                MFnDagNode fn(proxyShape->thisMObject());
                selectcommand += " \"";
                selectcommand += fn.fullPathName();
                selectcommand += "\"";
                deselectcommand += " \"";
                deselectcommand += fn.fullPathName();
                deselectcommand += "\"";

                if (hasSelectedItems) {
                    addSelection(selectcommand);
                }

                if (hasDeletedItems) {
                    MGlobal::executeCommandOnIdle(deselectcommand, false);
                }
            } break;
            }

            MString    final_command = "AL_usdmaya_ProxyShapePostSelect \"";
            MFnDagNode fn(proxyShape->thisMObject());
            final_command += fn.fullPathName();
            final_command += "\"";
            proxyShape->setChangedSelectionState(true);
            MGlobal::executeCommandOnIdle(final_command, false);
#if defined(WANT_UFE_BUILD)
        } // else MAYA_WANT_UFE_SELECTION
#endif
    }

    // restore clear colour
    glClearColor(clearCol[0], clearCol[1], clearCol[2], clearCol[3]);

    return selected;
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
