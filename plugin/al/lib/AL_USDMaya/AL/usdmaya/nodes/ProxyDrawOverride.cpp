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
#include <pxr/imaging/glf/glew.h>

#include "AL/usdmaya/nodes/ProxyDrawOverride.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/Engine.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <maya/M3dView.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDagNode.h>
#include <maya/MTime.h>

#if MAYA_API_VERSION >= 20180600
#include <maya/MPointArray.h>
#include <maya/MSelectionContext.h>
#endif

#if defined(WANT_UFE_BUILD)
#include "AL/usdmaya/TypeIDs.h"
#include "ufe/globalSelection.h"
#include "ufe/log.h"
#include "ufe/observableSelection.h"
#include "ufe/runTimeMgr.h"
#include "ufe/sceneItem.h"

#include <pxr/base/arch/env.h>
#endif

namespace AL {
namespace usdmaya {
namespace nodes {
namespace {
//----------------------------------------------------------------------------------------------------------------------
/// \brief  user data struct - holds the info needed to render the scene
//----------------------------------------------------------------------------------------------------------------------
class RenderUserData : public MUserData
{
public:
    // Constructor to use when shape is drawn but no bounding box.
    RenderUserData()
        : MUserData(false)
    {
    }

    // Make sure everything gets freed!
    ~RenderUserData() { }

    UsdImagingGLRenderParams m_params;
    UsdPrim                  m_rootPrim;
    ProxyShape*              m_shape = 0;
    MDagPath                 m_objPath;
};
} // namespace

//----------------------------------------------------------------------------------------------------------------------
MString ProxyDrawOverride::kDrawDbClassification("drawdb/geometry/AL_usdmaya");
MString ProxyDrawOverride::kDrawRegistrantId("pxrUsd");
MUint64 ProxyDrawOverride::s_lastRefreshFrameStamp = 0;

//----------------------------------------------------------------------------------------------------------------------
ProxyDrawOverride::ProxyDrawOverride(const MObject& obj)
#if MAYA_API_VERSION >= 20190000
    : MHWRender::MPxDrawOverride(obj, draw, true)
#elif MAYA_API_VERSION >= 20180600
    : MHWRender::MPxDrawOverride2(obj, draw, true)
#else
    : MHWRender::MPxDrawOverride(obj, draw, true)
#endif
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::ProxyDrawOverride\n");
}

//----------------------------------------------------------------------------------------------------------------------
ProxyDrawOverride::~ProxyDrawOverride() { }

//----------------------------------------------------------------------------------------------------------------------
MHWRender::MPxDrawOverride* ProxyDrawOverride::creator(const MObject& obj)
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::creator\n");
    return new ProxyDrawOverride(obj);
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyDrawOverride::isBounded(const MDagPath& objPath, const MDagPath& cameraPath) const
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::isBounded\n");
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
MBoundingBox
ProxyDrawOverride::boundingBox(const MDagPath& objPath, const MDagPath& cameraPath) const
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::boundingBox\n");
    ProxyShape* pShape = getShape(objPath);
    if (!pShape) {
        return MBoundingBox();
    }
    return pShape->boundingBox();
}

//----------------------------------------------------------------------------------------------------------------------
MUserData* ProxyDrawOverride::prepareForDraw(
    const MDagPath&                 objPath,
    const MDagPath&                 cameraPath,
    const MHWRender::MFrameContext& frameContext,
    MUserData*                      userData)
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::prepareForDraw\n");
    MFnDagNode fn(objPath);

    auto data = static_cast<RenderUserData*>(userData);
    auto shape = static_cast<ProxyShape*>(fn.userNode());
    if (!shape)
        return nullptr;

    auto engine = shape->engine();
    if (!engine) {
        return nullptr;
    }

    RenderUserData* newData = nullptr;
    if (data == nullptr) {
        data = newData = new RenderUserData;
    }

    if (!shape->getRenderAttris(data->m_params, frameContext, objPath)) {
        delete newData;
        return nullptr;
    }

    data->m_objPath = objPath;
    data->m_shape = shape;
    data->m_rootPrim = shape->getRootPrim();

    return data;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyDrawOverride::draw(const MHWRender::MDrawContext& context, const MUserData* data)
{
    TF_DEBUG(ALUSDMAYA_DRAW).Msg("ProxyDrawOverride::draw\n");

    float clearCol[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearCol);

    RenderUserData* ptr = (RenderUserData*)data;
    if (ptr && ptr->m_rootPrim) {
        ptr->m_shape->onRedraw();
        auto* engine = ptr->m_shape->engine();
        if (!engine) {
            TF_DEBUG(ALUSDMAYA_DRAW)
                .Msg("ProxyDrawOverride::draw - Error constructing usd opengl drawing engine - "
                     "aborting draw\n");
            return;
        }
        MHWRender::MStateManager*         stateManager = context.getStateManager();
        MHWRender::MDepthStencilStateDesc depthDesc;

        auto depthState = MHWRender::MStateManager::acquireDepthStencilState(depthDesc);
        auto previousDepthState = stateManager->getDepthStencilState();
        stateManager->setDepthStencilState(depthState);

        float minR, maxR;
        context.getDepthRange(minR, maxR);

        MHWRender::MDrawContext::LightFilter considerAllSceneLights
            = MHWRender::MDrawContext::kFilteredToLightLimit;

        uint32_t numLights = context.numberOfActiveLights(considerAllSceneLights);

        GlfSimpleLightVector lights;
        lights.reserve(numLights);
        for (uint32_t i = 0; i < numLights; ++i) {
            MFloatPointArray positions;
            MFloatVector     direction;
            float            intensity;
            MColor           color;
            bool             hasDirection;
            bool             hasPosition;
            context.getLightInformation(
                i,
                positions,
                direction,
                intensity,
                color,
                hasDirection,
                hasPosition,
                considerAllSceneLights);
            GlfSimpleLight light;
            if (hasPosition) {
                if (positions.length() == 1) {
                    GfVec4f pos(positions[0].x, positions[0].y, positions[0].z, positions[0].w);
                    light.SetPosition(pos);
                } else {
                    MFloatPoint fp(0, 0, 0);
                    for (uint32_t j = 0; j < positions.length(); ++j) {
                        fp += positions[j];
                    }
                    float value = (1.0f / positions.length());
                    fp.x *= value;
                    fp.y *= value;
                    fp.z *= value;
                    light.SetPosition(GfVec4f(fp.x, fp.y, fp.z, 1.0f));
                }
            }

            if (hasDirection) {
                GfVec3f dir(direction.x, direction.y, direction.z);
                light.SetSpotDirection(dir);
            }

            MHWRender::MLightParameterInformation* lightParam
                = context.getLightParameterInformation(i, considerAllSceneLights);
            if (lightParam) {
                MStringArray paramNames;
                lightParam->parameterList(paramNames);
                for (uint32_t i = 0, n = paramNames.length(); i != n; ++i) {
                    auto semantic = lightParam->parameterSemantic(paramNames[i]);
                    switch (semantic) {
                    case MHWRender::MLightParameterInformation::kIntensity: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                    } break;

                    case MHWRender::MLightParameterInformation::kColor: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                        if (fa.length() == 3) {
                            GfVec4f c(
                                intensity * fa[0], intensity * fa[1], intensity * fa[2], 1.0f);
                            light.SetDiffuse(c);
                            light.SetSpecular(c);
                        }
                    } break;
                    case MHWRender::MLightParameterInformation::kEmitsDiffuse:
                    case MHWRender::MLightParameterInformation::kEmitsSpecular: {
                        MIntArray ia;
                        lightParam->getParameter(paramNames[i], ia);
                    } break;
                    case MHWRender::MLightParameterInformation::kDecayRate: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                        if (fa[0] == 0) {
                            light.SetAttenuation(GfVec3f(1.0f, 0.0f, 0.0f));
                        } else if (fa[0] == 1) {
                            light.SetAttenuation(GfVec3f(0.0f, 1.0f, 0.0f));
                        } else if (fa[0] == 2) {
                            light.SetAttenuation(GfVec3f(0.0f, 0.0f, 1.0f));
                        }
                    } break;
                    case MHWRender::MLightParameterInformation::kDropoff: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                        light.SetSpotFalloff(fa[0]);
                    } break;
                    case MHWRender::MLightParameterInformation::kCosConeAngle: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                        fa[0] = acos(fa[0]) * 57.295779506f;
                        light.SetSpotCutoff(fa[0]);
                    } break;
                    case MHWRender::MLightParameterInformation::kStartShadowParameters:
                    case MHWRender::MLightParameterInformation::kShadowBias: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                    } break;
                    case MHWRender::MLightParameterInformation::kShadowMapSize:
                    case MHWRender::MLightParameterInformation::kShadowViewProj: {
                        MMatrix value;
                        lightParam->getParameter(paramNames[i], value);
                        GfMatrix4d m(value.matrix);
#if HDX_API_VERSION >= 6
                        light.SetShadowMatrices({ m });
#else
                        light.SetShadowMatrix(m);
#endif
                    } break;
                    case MHWRender::MLightParameterInformation::kShadowColor: {
                        MFloatArray fa;
                        lightParam->getParameter(paramNames[i], fa);
                        if (fa.length() == 3) {
                            GfVec4f c(fa[0], fa[1], fa[2], 1.0f);
                        }
                    } break;
                    case MHWRender::MLightParameterInformation::kGlobalShadowOn:
                    case MHWRender::MLightParameterInformation::kShadowOn: {
                        MIntArray ia;
                        lightParam->getParameter(paramNames[i], ia);
                        if (ia.length())
                            light.SetHasShadow(ia[0]);
                    } break;
                    default: break;
                    }
                }

                MStatus  status;
                MDagPath lightPath = lightParam->lightPath(&status);
                if (status) {
                    MMatrix wsm = lightPath.inclusiveMatrix();
                    light.SetIsCameraSpaceLight(false);
                    GfMatrix4d tm(wsm.inverse().matrix);
                    light.SetTransform(tm);
                } else {
                    light.SetIsCameraSpaceLight(true);
                }
                lights.push_back(light);
            }
        }

        auto getColour = [](const MPlug& p) {
            GfVec4f col(0, 0, 0, 1.0f);
            MStatus status;
            MPlug   pr = p.child(0, &status);
            if (status)
                col[0] = pr.asFloat();
            MPlug pg = p.child(1, &status);
            if (status)
                col[1] = pg.asFloat();
            MPlug pb = p.child(2, &status);
            if (status)
                col[2] = pb.asFloat();
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

        engine->SetLightingState(lights, material, GfVec4f(0.05f));
        glDepthFunc(GL_LESS);

        int originX, originY, width, height;
        context.getViewportDimensions(originX, originY, width, height);

        engine->SetCameraState(
            GfMatrix4d(context.getMatrix(MHWRender::MFrameContext::kViewMtx).matrix),
            GfMatrix4d(context.getMatrix(MHWRender::MFrameContext::kProjectionMtx).matrix));
        engine->SetRenderViewport(GfVec4d(originX, originY, width, height));

        engine->SetRootTransform(GfMatrix4d(ptr->m_objPath.inclusiveMatrix().matrix));

        auto          view = M3dView::active3dView();
        const auto&   paths1 = ptr->m_shape->selectedPaths();
        const auto&   paths2 = ptr->m_shape->selectionList().paths();
        SdfPathVector combined;
        combined.reserve(paths1.size() + paths2.size());
        combined.insert(combined.end(), paths1.begin(), paths1.end());
        combined.insert(combined.end(), paths2.begin(), paths2.end());

        ptr->m_params.frame = ptr->m_shape->outTimePlug().asMTime().as(MTime::uiUnit());
        engine->Render(ptr->m_rootPrim, ptr->m_params);

        if (combined.size()) {
            UsdImagingGLRenderParams params = ptr->m_params;
            params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
            MColor colour = M3dView::leadColor();
            params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);
            glDepthFunc(GL_LEQUAL);
            // Geometry already rendered, can't offset it deeper.  Push
            // lines in front with negative offset.
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-1.0, -1.0);
            engine->RenderBatch(combined, params);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }

#if defined(WANT_UFE_BUILD)
        if (ArchHasEnv("MAYA_WANT_UFE_SELECTION")) {
            // Draw selection highlighting for all USD items in the UFE selection.
            SdfPathVector ufePaths;
            auto          ufeSelList = Ufe::GlobalSelection::get();

            Ufe::PathSegment proxyUfePath = ptr->m_shape->ufePathSegment();
            for (const auto& sceneItem : *ufeSelList) {
                if (sceneItem->runTimeId() == USD_UFE_RUNTIME_ID) {
                    const Ufe::Path&        itemPath = sceneItem->path();
                    const Ufe::PathSegment& usdPathSegment = itemPath.getSegments().back();
                    if (usdPathSegment.runTimeId() == USD_UFE_RUNTIME_ID
                        && itemPath.getSegments().size() == 2) {
                        const Ufe::PathSegment& mayaPathSegment = itemPath.getSegments().front();
                        if (mayaPathSegment == proxyUfePath) {
                            ufePaths.emplace_back(usdPathSegment.string());
                        }
                    }
                }
            }
            if (!ufePaths.empty()) {
                UsdImagingGLRenderParams params = ptr->m_params;
                params.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
                MColor colour = M3dView::leadColor(); // Maya selection color
                params.wireframeColor = GfVec4f(colour.r, colour.g, colour.b, 1.0f);
                glDepthFunc(GL_LEQUAL);
                // Geometry already rendered, can't offset it deeper.  Push
                // lines in front with negative offset.
                glEnable(GL_POLYGON_OFFSET_LINE);
                glPolygonOffset(-1.0, -1.0);
                engine->RenderBatch(ufePaths, params);
                glDisable(GL_POLYGON_OFFSET_LINE);
            }
        }
#endif

        // HACK: Maya doesn't restore this ONE buffer binding after our override is done so we have
        // to do it for them.
        glBindBufferBase(GL_UNIFORM_BUFFER, 4, uboBinding);

        stateManager->setDepthStencilState(previousDepthState);
        MHWRender::MStateManager::releaseDepthStencilState(depthState);

        // Check framestamp b/c we don't want to put multiple refresh commands
        // on the idle queue for a single frame-render... especially if we have
        // multiple ProxyShapes...
        if (!engine->IsConverged() && context.getFrameStamp() != s_lastRefreshFrameStamp) {
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
    MObject           obj = objPath.node();
    MFnDependencyNode dnNode(obj);
    if (obj.apiType() != MFn::kPluginShape) {
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

#if MAYA_API_VERSION >= 20180600
//----------------------------------------------------------------------------------------------------------------------
bool ProxyDrawOverride::userSelect(
    const MHWRender::MSelectionInfo& selectInfo,
    const MHWRender::MDrawContext&   context,
    const MDagPath&                  objPath,
    const MUserData*                 data,
    MSelectionList&                  selectionList,
    MPointArray&                     worldSpaceHitPts)
{
    TF_DEBUG(ALUSDMAYA_SELECTION).Msg("ProxyDrawOverride::userSelect\n");

    MString fullSelPath = objPath.fullPathName();

    if (!MGlobal::optionVarIntValue("AL_usdmaya_selectionEnabled"))
        return false;

    MString        selectionMaskName(ProxyShape::s_selectionMaskName);
    MSelectionMask mask(selectionMaskName);
    if (!selectInfo.selectable(mask))
        return false;

    MStatus status;
    MMatrix worldViewMatrix = context.getMatrix(MHWRender::MFrameContext::kWorldViewMtx, &status);
    if (status != MStatus::kSuccess)
        return false;

    MMatrix projectionMatrix = context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    if (status != MStatus::kSuccess)
        return false;

    // Compute a pick matrix that, when it is post-multiplied with the projection
    // matrix, will cause the picking region to fill the entire viewport for
    // OpenGL selection.
    {
        int view_x, view_y, view_w, view_h;
        context.getViewportDimensions(view_x, view_y, view_w, view_h);

        unsigned int sel_x, sel_y, sel_w, sel_h;
        selectInfo.selectRect(sel_x, sel_y, sel_w, sel_h);

        double center_x = sel_x + sel_w * 0.5;
        double center_y = sel_y + sel_h * 0.5;

        MMatrix pickMatrix;
        pickMatrix[0][0] = view_w / double(sel_w);
        pickMatrix[1][1] = view_h / double(sel_h);
        pickMatrix[3][0] = (view_w - 2.0 * (center_x - view_x)) / double(sel_w);
        pickMatrix[3][1] = (view_h - 2.0 * (center_y - view_y)) / double(sel_h);

        projectionMatrix *= pickMatrix;
    }

    // Get world to local matrix
    MMatrix    invMatrix = objPath.inclusiveMatrixInverse();
    GfMatrix4d worldToLocalSpace(invMatrix.matrix);

    auto* proxyShape = static_cast<ProxyShape*>(getShape(objPath));
    auto  engine = proxyShape->engine();
    if (!engine)
        return false;

    // The commands we execute inside this function shouldn't do special
    // processing of the proxy we are currently handling here if they
    // should run across it.
    proxyShape->m_pleaseIgnoreSelection = true;

    UsdImagingGLRenderParams params;
    // Mostly want to get render params to set renderGuides/proxyGuides/etc
    proxyShape->getRenderAttris(params, context, objPath);

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

    bool hitSelected = engine->TestIntersectionBatch(
        GfMatrix4d(worldViewMatrix.matrix),
        GfMatrix4d(projectionMatrix.matrix),
        worldToLocalSpace,
        rootPath,
        params,
        resolution,
        ProxyDrawOverrideSelectionHelper::path_ting,
        &hitBatch);

    auto selected = false;

    auto addSelection = [&hitBatch, &selectionList, &worldSpaceHitPts, proxyShape, &selected](
                            const MString& command) {
        selected = true;
        MStringArray nodes;
        MGlobal::executeCommand(command, nodes, false, true);

        for (const auto& it : hitBatch) {
            auto path = it.first;
            auto obj = proxyShape->findRequiredPath(path);
            if (obj != MObject::kNullObj) {
                MFnDagNode dagNode(obj);
                MDagPath   dg;
                dagNode.getPath(dg);
                const double* p = it.second.worldSpaceHitPoint.GetArray();

                selectionList.add(dg);
                worldSpaceHitPts.append(MPoint(p[0], p[1], p[2]));
            }
        }
    };

    // Maya determines the selection list adjustment mode by Ctrl/Shift modifiers.
    int modifiers = 0;
    MGlobal::executeCommand("getModifiers", modifiers);

    const bool shiftHeld = (modifiers % 2);
    const bool ctrlHeld = (modifiers / 4 % 2);

    MGlobal::ListAdjustment listAdjustment = MGlobal::kReplaceList;
    if (shiftHeld && ctrlHeld) {
        listAdjustment = MGlobal::kAddToList;
    } else if (ctrlHeld) {
        listAdjustment = MGlobal::kRemoveFromList;
    } else if (shiftHeld) {
        listAdjustment = MGlobal::kXORWithList;
    }

    // Currently we have two approaches to selection. One method works with undo (but does not
    // play nicely with maya geometry). The second method doesn't work with undo, but does play
    // nicely with maya geometry.
    const int selectionMode = MGlobal::optionVarIntValue("AL_usdmaya_selectMode");
    if (1 == selectionMode) {
        if (hitSelected) {
            MString command = "AL_usdmaya_ProxyShapeSelect";
            switch (listAdjustment) {
            case MGlobal::kReplaceList: command += " -r"; break;
            case MGlobal::kRemoveFromList: command += " -d"; break;
            case MGlobal::kXORWithList: command += " -tgl"; break;
            case MGlobal::kAddToList: command += " -a"; break;
            case MGlobal::kAddToHeadOfList: /* should never get here */ break;
            }

            for (const auto& it : hitBatch) {
                auto path = it.first;
                command += " -pp \"";
                command += path.GetText();
                command += "\"";
            }

            command += " \"";
            command += fullSelPath;
            command += "\"";
            MGlobal::executeCommandOnIdle(command, false);
        } else {
            MString command = "AL_usdmaya_ProxyShapeSelect -cl ";
            command += " \"";
            command += fullSelPath;
            command += "\"";
            MGlobal::executeCommandOnIdle(command, false);
        }
    } else {
        SdfPathVector paths;
        if (!hitBatch.empty()) {
            paths.reserve(hitBatch.size());

            auto addHit
                = [&paths](Engine::HitBatch::const_reference& it) { paths.push_back(it.first); };

            // Due to the inaccuracies in the selection method in gl engine
            // we still need to find the closest selection.
            // Around the edges it often selects two or more prims.
            if (selectInfo.singleSelection()) {
                auto closestHit = hitBatch.cbegin();

                if (hitBatch.size() > 1) {
                    MDagPath cameraPath;
                    M3dView::active3dView().getCamera(cameraPath);
                    const auto cameraPoint
                        = cameraPath.inclusiveMatrix() * MPoint(0.0, 0.0, 0.0, 1.0);
                    auto distanceToCameraSq
                        = [&cameraPoint](Engine::HitBatch::const_reference& it) -> double {
                        const auto dx = cameraPoint.x - it.second.worldSpaceHitPoint[0];
                        const auto dy = cameraPoint.y - it.second.worldSpaceHitPoint[1];
                        const auto dz = cameraPoint.z - it.second.worldSpaceHitPoint[2];
                        return dx * dx + dy * dy + dz * dz;
                    };

                    auto closestDistance = distanceToCameraSq(*closestHit);
                    for (auto it = ++hitBatch.cbegin(), itEnd = hitBatch.cend(); it != itEnd;
                         ++it) {
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

#if defined(WANT_UFE_BUILD)
        if (ArchHasEnv("MAYA_WANT_UFE_SELECTION")) {
            // Get the Hierarchy Handler of USD - Id = 2
            Ufe::HierarchyHandler::Ptr handler = Ufe::RunTimeMgr::instance().hierarchyHandler(2);
            if (handler == nullptr) {
                MGlobal::displayError(
                    "USD Hierarchy handler has not been loaded - Picking is not possible");
                return false;
            }

            if (paths.size()) {
                auto globalSelection = Ufe::GlobalSelection::get();

                for (const auto& it : paths) {
                    // Build a path segment of the USD picked object
                    Ufe::PathSegment ps_usd(it.GetText(), 2, '/');

                    // Create a sceneItem
                    const Ufe::SceneItem::Ptr& si { handler->createItem(
                        proxyShape->ufePath() + ps_usd) };

                    switch (listAdjustment) {
                    case MGlobal::kReplaceList:
                        // The list has been cleared before viewport selection runs, so we
                        // can add the new hits directly. UFE selection list is a superset
                        // of Maya selection list, calling clear()/replaceWith() on UFE
                        // selection list would clear Maya selection list.
                        globalSelection->append(si);
                        break;
                    case MGlobal::kAddToList: globalSelection->append(si); break;
                    case MGlobal::kRemoveFromList: globalSelection->remove(si); break;
                    case MGlobal::kXORWithList:
                        if (!globalSelection->remove(si)) {
                            globalSelection->append(si);
                        }
                        break;
                    case MGlobal::kAddToHeadOfList:
                        // No such operation on UFE selection.
                        UFE_LOG("UFE does not support prepend to selection.");
                        break;
                    }
                }
            }
        } else {
#endif
            switch (listAdjustment) {
            case MGlobal::kReplaceList: {
                MString command;
                if (!proxyShape->selectedPaths().empty()) {
                    command = "AL_usdmaya_ProxyShapeSelect -i -cl ";
                    command += " \"";
                    command += fullSelPath;
                    command += "\";";
                }

                if (!paths.empty()) {
                    command += "AL_usdmaya_ProxyShapeSelect -i -a ";
                    for (const auto& it : paths) {
                        command += " -pp \"";
                        command += it.GetText();
                        command += "\"";
                    }
                    command += " \"";
                    command += fullSelPath;
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
                    command += " \"";
                    command += fullSelPath;
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
                    command += " \"";
                    command += fullSelPath;
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
                selectcommand += " \"";
                selectcommand += fullSelPath;
                selectcommand += "\"";
                deselectcommand += " \"";
                deselectcommand += fullSelPath;
                deselectcommand += "\"";

                if (hasSelectedItems) {
                    addSelection(selectcommand);
                }

                if (hasDeletedItems) {
                    MGlobal::executeCommandOnIdle(deselectcommand, false);
                }
            } break;
            }

            MString final_command = "AL_usdmaya_ProxyShapePostSelect \"";
            final_command += fullSelPath;
            final_command += "\"";
            proxyShape->setChangedSelectionState(true);
            MGlobal::executeCommandOnIdle(final_command, false);
#if defined(WANT_UFE_BUILD)
        } // else MAYA_WANT_UFE_SELECTION
#endif
    }

    ProxyDrawOverrideSelectionHelper::m_paths.clear();

    // We are done executing commands that needed to handle our current
    // proxy as a special case.  Unset the ignore state on the proxy.
    proxyShape->m_pleaseIgnoreSelection = false;

    return selected;
}
#endif

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
