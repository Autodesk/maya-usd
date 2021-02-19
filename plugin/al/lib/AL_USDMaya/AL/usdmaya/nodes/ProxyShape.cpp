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
/*
#include <pxr/usdImaging/usdImaging/delegate.h>
#include <pxr/usdImaging/usdImaging/version.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>

*/

#include "AL/maya/utils/Utils.h"
#include "AL/usd/transaction/TransactionManager.h"
#include "AL/usdmaya/CodeTimings.h"
#include "AL/usdmaya/Global.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/Version.h"
#include "AL/usdmaya/cmds/ProxyShapePostLoadProcess.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/Engine.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/RendererManager.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/utils/Utils.h"

#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/stageData.h>
#include <mayaUsd/utils/utilFileSystem.h>

#include <pxr/usd/ar/resolver.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stageCacheContext.h>
#include <pxr/usd/usdGeom/imageable.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdUtils/stageCache.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MEvaluationNode.h>
#include <maya/MEventMessage.h>
#include <maya/MFileIO.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnReference.h>
#include <maya/MGlobal.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MNodeClass.h>
#include <maya/MTime.h>
#include <maya/MViewport2Renderer.h>

#include <boost/filesystem.hpp>

#if defined(WANT_UFE_BUILD)
#include "ufe/path.h"
#endif

namespace AL {
namespace usdmaya {
namespace nodes {
typedef void (
    *proxy_function_prototype)(void* userData, AL::usdmaya::nodes::ProxyShape* proxyInstance);

const char* ProxyShape::s_selectionMaskName = "al_ProxyShape";

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialiseTranslatorContext()
{
    triggerEvent("PreSerialiseContext");

    context()->updateUniqueKeys();
    serializedTrCtxPlug().setValue(context()->serialise());

    triggerEvent("PostSerialiseContext");
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::deserialiseTranslatorContext()
{
    triggerEvent("PreDeserialiseContext");

    MString value;
    serializedTrCtxPlug().getValue(value);
    context()->deserialise(value);

    triggerEvent("PostDeserialiseContext");
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(ProxyShape, AL_USDMAYA_PROXYSHAPE, AL_usdmaya);

MObject ProxyShape::m_populationMaskIncludePaths = MObject::kNullObj;
MObject ProxyShape::m_excludedTranslatedGeometry = MObject::kNullObj;
MObject ProxyShape::m_timeOffset = MObject::kNullObj;
MObject ProxyShape::m_timeScalar = MObject::kNullObj;
MObject ProxyShape::m_layers = MObject::kNullObj;
MObject ProxyShape::m_serializedSessionLayer = MObject::kNullObj;
MObject ProxyShape::m_sessionLayerName = MObject::kNullObj;
MObject ProxyShape::m_serializedTrCtx = MObject::kNullObj;
MObject ProxyShape::m_unloaded = MObject::kNullObj;
MObject ProxyShape::m_ambient = MObject::kNullObj;
MObject ProxyShape::m_diffuse = MObject::kNullObj;
MObject ProxyShape::m_specular = MObject::kNullObj;
MObject ProxyShape::m_emission = MObject::kNullObj;
MObject ProxyShape::m_shininess = MObject::kNullObj;
MObject ProxyShape::m_serializedRefCounts = MObject::kNullObj;
MObject ProxyShape::m_version = MObject::kNullObj;
MObject ProxyShape::m_transformTranslate = MObject::kNullObj;
MObject ProxyShape::m_transformRotate = MObject::kNullObj;
MObject ProxyShape::m_transformScale = MObject::kNullObj;
MObject ProxyShape::m_stageDataDirty = MObject::kNullObj;
MObject ProxyShape::m_assetResolverConfig = MObject::kNullObj;
MObject ProxyShape::m_variantFallbacks = MObject::kNullObj;
MObject ProxyShape::m_visibleInReflections = MObject::kNullObj;
MObject ProxyShape::m_visibleInRefractions = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
std::vector<MObjectHandle> ProxyShape::m_unloadedProxyShapes;
//----------------------------------------------------------------------------------------------------------------------
UsdPrim ProxyShape::getUsdPrim(MDataBlock& dataBlock) const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getUsdPrim\n");
    return _GetUsdPrim(dataBlock);
}

//----------------------------------------------------------------------------------------------------------------------
UsdStagePopulationMask ProxyShape::constructStagePopulationMask(const MString& paths) const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION)
        .Msg("ProxyShape::constructStagePopulationMask(%s)\n", paths.asChar());
    UsdStagePopulationMask mask;
    SdfPathVector          list = getPrimPathsFromCommaJoinedString(paths);
    if (list.empty()) {
        TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape: No mask specified, will mask none.\n");
        return UsdStagePopulationMask::All();
    }

    for (const SdfPath& path : list) {
        TF_DEBUG(ALUSDMAYA_EVALUATION)
            .Msg("ProxyShape: Add include to mask:(%s)\n", path.GetText());
        mask.Add(path);
    }
    return mask;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::translatePrimPathsIntoMaya(
    const SdfPathVector&                             importPaths,
    const SdfPathVector&                             teardownPaths,
    const fileio::translators::TranslatorParameters& param)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION)
        .Msg(
            "ProxyShape:translatePrimPathsIntoMaya ImportSize='%zd' TearDownSize='%zd' \n",
            importPaths.size(),
            teardownPaths.size());

    // Resolve SdfPathSet to UsdPrimVector
    UsdPrimVector importPrims;
    for (const SdfPath& path : importPaths) {
        UsdPrim prim = m_stage->GetPrimAtPath(path);
        if (prim.IsValid()) {
            importPrims.push_back(prim);
        } else {
            TF_DEBUG(ALUSDMAYA_EVALUATION)
                .Msg(
                    "ProxyShape:translatePrimPathsIntoMaya Path for import '%s' resolves to an "
                    "invalid prim\n",
                    path.GetText());
        }
    }

    translatePrimsIntoMaya(importPrims, teardownPaths, param);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::translatePrimsIntoMaya(
    const UsdPrimVector&                             importPrims,
    const SdfPathVector&                             teardownPrims,
    const fileio::translators::TranslatorParameters& param)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION)
        .Msg(
            "ProxyShape:translatePrimsIntoMaya ImportSize='%zd' TearDownSize='%zd' \n",
            importPrims.size(),
            teardownPrims.size());

    proxy::PrimFilter filter(teardownPrims, importPrims, this, param.forceTranslatorImport());

    if (TfDebug::IsEnabled(ALUSDMAYA_TRANSLATORS)) {
        std::cout << "new prims" << std::endl;
        for (auto it : filter.newPrimSet()) {
            std::cout << it.GetPath().GetText() << ' ' << it.GetTypeName().GetText() << std::endl;
        }
        std::cout << "new transforms" << std::endl;
        for (auto it : filter.transformsToCreate()) {
            std::cout << it.GetPath().GetText() << ' ' << it.GetTypeName().GetText() << std::endl;
        }
        std::cout << "updateable prims" << std::endl;
        for (auto it : filter.updatablePrimSet()) {
            std::cout << it.GetPath().GetText() << '\n';
        }
        std::cout << "removed prims" << std::endl;
        for (auto it : filter.removedPrimSet()) {
            std::cout << it.GetText() << '\n';
        }
    }

    // content to remove needs to be dealt with first
    // as some nodes might be re-imported and we have
    // to make sure their "old" version is gone before
    // recreating them.
    context()->removeEntries(filter.removedPrimSet());

    cmds::ProxyShapePostLoadProcess::MObjectToPrim objsToCreate;
    if (!filter.transformsToCreate().empty()) {
        cmds::ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims(
            this,
            filter.transformsToCreate(),
            parentTransform(),
            objsToCreate,
            param.pushToPrim(),
            param.readAnimatedValues());
    }

    if (!filter.newPrimSet().empty()) {
        cmds::ProxyShapePostLoadProcess::createSchemaPrims(this, filter.newPrimSet(), param);
    }

    if (!filter.updatablePrimSet().empty()) {
        cmds::ProxyShapePostLoadProcess::updateSchemaPrims(this, filter.updatablePrimSet());
    }

    cleanupTransformRefs();

    context()->updatePrimTypes();

    // now perform any post-creation fix up
    if (!filter.newPrimSet().empty()) {
        cmds::ProxyShapePostLoadProcess::connectSchemaPrims(this, filter.newPrimSet());
    }

    if (!filter.updatablePrimSet().empty()) {
        cmds::ProxyShapePostLoadProcess::connectSchemaPrims(this, filter.updatablePrimSet());
    }

    if (context()->isExcludedGeometryDirty()) {
        TF_DEBUG(ALUSDMAYA_EVALUATION)
            .Msg("ProxyShape:translatePrimsIntoMaya excluded geometry has been modified, "
                 "reconstructing imaging engine \n");
        constructExcludedPrims(); // if excluded prims changed, this will call
                                  // constructGLImagingEngine
    }
}
//----------------------------------------------------------------------------------------------------------------------
SdfPathVector ProxyShape::getPrimPathsFromCommaJoinedString(const MString& paths) const
{
    SdfPathVector result;
    if (paths.length()) {
        const char* begin = paths.asChar();
        const char* end = paths.asChar() + paths.length();
        const char* iter = std::find(begin, end, ',');
        while (iter != end) {
            result.push_back(SdfPath(std::string(begin, iter)));
            begin = iter + 1;
            iter = std::find(begin, end, ',');
        }

        result.push_back(SdfPath(std::string(begin, end)));
    }
    return result;
}

//----------------------------------------------------------------------------------------------------------------------
Engine* ProxyShape::engine(bool construct)
{
    if (!m_engine && construct) {
        constructGLImagingEngine();
    }
    return m_engine;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::destroyGLImagingEngine()
{
    if (m_engine) {
        triggerEvent("DestroyGLEngine");
        delete m_engine;
        m_engine = nullptr;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructGLImagingEngine()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::constructGLImagingEngine\n");

    // kBatch does not cover mayapy use, we only need this in interactive mode:
    if (MGlobal::mayaState() == MGlobal::kInteractive) {
        if (m_stage) {
            // function prototype of callback we wish to register
            typedef void (*proxy_function_prototype)(void*, AL::usdmaya::nodes::ProxyShape*);

            // delete previous instance
            destroyGLImagingEngine();

            const auto& translatedGeo = m_context->excludedGeometry();

            // combine the excluded paths
            SdfPathVector excludedGeometryPaths;
            excludedGeometryPaths.reserve(
                m_excludedTaggedGeometry.size() + m_excludedGeometry.size() + translatedGeo.size());
            excludedGeometryPaths.assign(
                m_excludedTaggedGeometry.begin(), m_excludedTaggedGeometry.end());
            excludedGeometryPaths.insert(
                excludedGeometryPaths.end(), m_excludedGeometry.begin(), m_excludedGeometry.end());
            for (auto& it : translatedGeo) {
                excludedGeometryPaths.push_back(it.second);
            }

            m_engine = new Engine(m_path, excludedGeometryPaths);
            // set renderer plugin based on RendererManager setting
            RendererManager* manager = RendererManager::findManager();
            if (manager && m_engine) {
                manager->changeRendererPlugin(this, true);
            }

            triggerEvent("ConstructGLEngine");
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::setDependentsDirty(const MPlug& plugBeingDirtied, MPlugArray& plugs)
{
    // I thought that, if your ProxyDrawOverride is set to not always be dirty,
    // prepareForDraw would automatically be triggered whenever any attribute
    // that was marked as affectsAppearance was dirtied; however, this is not
    // the case, so we mark the draw geo dirty ourselves.
    {
        MStatus      status;
        MFnAttribute attr(plugBeingDirtied, &status);
        if (!status) {
            if (plugBeingDirtied.isNull()) {
                TF_CODING_ERROR(TfStringPrintf(
                    "ProxyShape setInternalValue given invalid plug - shape: %s", name().asChar()));
            } else {
                TF_CODING_ERROR(TfStringPrintf(
                    "Unable to retrieve attribute from plug: %s",
                    plugBeingDirtied.name().asChar()));
            }
        } else {
            if (attr.affectsAppearance()) {
                MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());
            }
        }
    }

    if (plugBeingDirtied == time() || plugBeingDirtied == m_timeOffset
        || plugBeingDirtied == m_timeScalar) {
        plugs.append(outTimePlug());
        return MS::kSuccess;
    }
    if (plugBeingDirtied == filePath()) {
        MHWRender::MRenderer::setGeometryDrawDirty(thisMObject(), true);
    }

    if (plugBeingDirtied == outStageData() ||
        // All the plugs that affect outStageDataAttr
        plugBeingDirtied == filePath() || plugBeingDirtied == primPath()
        || plugBeingDirtied == m_populationMaskIncludePaths || plugBeingDirtied == m_stageDataDirty
        || plugBeingDirtied == m_assetResolverConfig) {
        MayaUsdProxyStageInvalidateNotice(*this).Send();
    }

    return MPxSurfaceShape::setDependentsDirty(plugBeingDirtied, plugs);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::preEvaluation(const MDGContext& context, const MEvaluationNode& evaluationNode)
{
    if (!context.isNormal())
        return MStatus::kFailure;
    return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::getRenderAttris(
    UsdImagingGLRenderParams&       attribs,
    const MHWRender::MFrameContext& drawRequest,
    const MDagPath&                 objPath)
{
    uint32_t displayStyle = drawRequest.getDisplayStyle();
    uint32_t displayStatus = MHWRender::MGeometryUtilities::displayStatus(objPath);

    // set wireframe colour
    MColor wireColour = MHWRender::MGeometryUtilities::wireframeColor(objPath);
    attribs.wireframeColor = GfVec4f(wireColour.r, wireColour.g, wireColour.b, wireColour.a);

    // determine the shading mode
    const uint32_t wireframeOnShaded1
        = (MHWRender::MFrameContext::kWireFrame | MHWRender::MFrameContext::kGouraudShaded);
    const uint32_t wireframeOnShaded2
        = (MHWRender::MFrameContext::kWireFrame | MHWRender::MFrameContext::kFlatShaded);
    if ((displayStyle & wireframeOnShaded1) == wireframeOnShaded1
        || (displayStyle & wireframeOnShaded2) == wireframeOnShaded2) {
        attribs.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
    } else if (displayStyle & MHWRender::MFrameContext::kWireFrame) {
        attribs.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME;
    } else if (displayStyle & MHWRender::MFrameContext::kFlatShaded) {
        attribs.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_FLAT;
        if ((displayStatus == MHWRender::kActive) || (displayStatus == MHWRender::kLead)
            || (displayStatus == MHWRender::kHilite)) {
            attribs.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
        }
    } else if (displayStyle & MHWRender::MFrameContext::kGouraudShaded) {
        attribs.drawMode = UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
        if ((displayStatus == MHWRender::kActive) || (displayStatus == MHWRender::kLead)
            || (displayStatus == MHWRender::kHilite)) {
            attribs.drawMode = UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE;
        }
    } else if (displayStyle & MHWRender::MFrameContext::kBoundingBox) {
        attribs.drawMode = UsdImagingGLDrawMode::DRAW_POINTS;
    }

    // determine whether to use the default material for everything
    attribs.enableSceneMaterials = !(displayStyle & MHWRender::MFrameContext::kDefaultMaterial);

    // set the time for the scene
    attribs.frame = outTimePlug().asMTime().as(MTime::uiUnit());

    if (displayStyle & MHWRender::MFrameContext::kBackfaceCulling) {
        attribs.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_BACK;
    } else {
        attribs.cullStyle = UsdImagingGLCullStyle::CULL_STYLE_NOTHING;
    }

    const float complexities[] = { 1.05f, 1.15f, 1.25f, 1.35f, 1.45f, 1.55f, 1.65f, 1.75f, 1.9f };
    attribs.complexity = complexities[complexityPlug().asInt()];
    attribs.showGuides = drawGuidePurposePlug().asBool();
    attribs.showProxy = drawProxyPurposePlug().asBool();
    attribs.showRender = drawRenderPurposePlug().asBool();
    return true;
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::ProxyShape()
    : MayaUsdProxyShapeBase()
    , AL::maya::utils::NodeHelper()
    , AL::event::NodeEvents(&AL::event::EventScheduler::getScheduler())
    , m_context(fileio::translators::TranslatorContext::create(this))
    , m_translatorManufacture(context())
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::ProxyShape\n");
    m_onSelectionChanged
        = MEventMessage::addEventCallback(MString("SelectionChanged"), onSelectionChanged, this);

    TfWeakPtr<ProxyShape> me(this);

    m_variantChangedNoticeKey = TfNotice::Register(me, &ProxyShape::variantSelectionListener);
    m_objectsChangedNoticeKey = TfNotice::Register(me, &ProxyShape::onObjectsChanged, m_stage);
    m_editTargetChanged = TfNotice::Register(me, &ProxyShape::onEditTargetChanged, m_stage);

    TfWeakPtr<UsdStage> stage(m_stage);
    m_transactionNoticeKey = TfNotice::Register(me, &ProxyShape::onTransactionNotice, stage);

    registerEvents();
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::~ProxyShape()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::~ProxyShape\n");
    triggerEvent("PreDestroyProxyShape");
    MEventMessage::removeCallback(m_onSelectionChanged);
    TfNotice::Revoke(m_variantChangedNoticeKey);
    TfNotice::Revoke(m_objectsChangedNoticeKey);
    TfNotice::Revoke(m_editTargetChanged);
    TfNotice::Revoke(m_transactionNoticeKey);
    destroyGLImagingEngine();
    triggerEvent("PostDestroyProxyShape");
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::initialise()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::initialise\n");

    MStatus retValue = inheritAttributesFrom(MayaUsdProxyShapeBase::typeName);
    CHECK_MSTATUS_AND_RETURN_IT(retValue);

    const char* errorString = "ProxyShape::initialize";
    try {
        setNodeType(kTypeName);
        addFrame("USD Proxy Shape Node");
        m_serializedSessionLayer = addStringAttr(
            "serializedSessionLayer", "ssl", kCached | kReadable | kWritable | kStorable | kHidden);
        m_sessionLayerName = addStringAttr(
            "sessionLayerName", "sln", kCached | kReadable | kWritable | kStorable | kHidden);

        // for backward compatibility (or at least to stop maya spewing out errors on scene open).
        // This attribute was removed in 0.32.17
        addStringAttr("serializedArCtx", "arcd", kReadable | kWritable | kHidden);
        // m_filePath / m_primPath / m_excludePrimPaths are internal just so we get notification on
        // change
        inheritFilePathAttr(
            "filePath",
            kCached | kReadable | kWritable | kStorable | kAffectsAppearance | kInternal,
            kLoad,
            "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc)");

        inheritStringAttr(
            "primPath",
            kCached | kReadable | kWritable | kStorable | kAffectsAppearance | kInternal);
        inheritStringAttr(
            "excludePrimPaths",
            kCached | kReadable | kWritable | kStorable | kAffectsAppearance | kInternal);
        m_populationMaskIncludePaths = addStringAttr(
            "populationMaskIncludePaths",
            "pmi",
            kCached | kReadable | kWritable | kStorable | kAffectsAppearance);
        m_excludedTranslatedGeometry = addStringAttr(
            "excludedTranslatedGeometry",
            "etg",
            kCached | kReadable | kWritable | kStorable | kAffectsAppearance);

        inheritInt32Attr(
            "complexity",
            kCached | kConnectable | kReadable | kWritable | kAffectsAppearance | kKeyable
                | kStorable);
        // outStageData attribute already added in base class.
        inheritBoolAttr(
            "drawGuidePurpose", kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
        inheritBoolAttr(
            "drawProxyPurpose", kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
        inheritBoolAttr(
            "drawRenderPurpose", kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
        m_unloaded = addBoolAttr(
            "unloaded",
            "ul",
            false,
            kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
        m_serializedTrCtx
            = addStringAttr("serializedTrCtx", "srtc", kReadable | kWritable | kStorable | kHidden);

        addFrame("USD Timing Information");
        inheritTimeAttr(
            "time",
            kCached | kConnectable | kReadable | kWritable | kStorable | kAffectsAppearance);
        m_timeOffset = addTimeAttr(
            "timeOffset",
            "tmo",
            MTime(0.0),
            kCached | kConnectable | kReadable | kWritable | kStorable | kAffectsAppearance);
        m_timeScalar = addDoubleAttr(
            "timeScalar",
            "tms",
            1.0,
            kCached | kConnectable | kReadable | kWritable | kStorable | kAffectsAppearance);
        inheritTimeAttr("outTime", kCached | kConnectable | kReadable | kAffectsAppearance);
        m_layers = addMessageAttr("layers", "lys", kWritable | kReadable | kConnectable | kHidden);

        addFrame("OpenGL Display");
        m_ambient = addColourAttr(
            "ambientColour",
            "amc",
            MColor(0.1f, 0.1f, 0.1f),
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
        m_diffuse = addColourAttr(
            "diffuseColour",
            "dic",
            MColor(0.7f, 0.7f, 0.7f),
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
        m_specular = addColourAttr(
            "specularColour",
            "spc",
            MColor(0.6f, 0.6f, 0.6f),
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
        m_emission = addColourAttr(
            "emissionColour",
            "emc",
            MColor(0.0f, 0.0f, 0.0f),
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
        m_shininess = addFloatAttr(
            "shininess",
            "shi",
            5.0f,
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);

        m_serializedRefCounts = addStringAttr(
            "serializedRefCounts", "strcs", kReadable | kWritable | kStorable | kHidden);

        m_version = addStringAttr("version", "vrs", getVersion(), kReadable | kStorable | kHidden);

        MNodeClass ncTransform("transform");
        m_transformTranslate = ncTransform.attribute("t");
        m_transformRotate = ncTransform.attribute("r");
        m_transformScale = ncTransform.attribute("s");

        MNodeClass ncProxyShape(kTypeId);
        m_visibleInReflections = ncProxyShape.attribute("visibleInReflections");
        m_visibleInRefractions = ncProxyShape.attribute("visibleInRefractions");

        m_stageDataDirty = addBoolAttr(
            "stageDataDirty", "sdd", false, kWritable | kAffectsAppearance | kInternal);

        inheritInt32Attr("stageCacheId", kCached | kConnectable | kReadable | kInternal);

        m_assetResolverConfig = addStringAttr(
            "assetResolverConfig",
            "arc",
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance | kInternal);
        m_variantFallbacks = addStringAttr(
            "variantFallbacks",
            "vfs",
            kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance | kInternal);

        AL_MAYA_CHECK_ERROR(attributeAffects(time(), outTime()), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_timeOffset, outTime()), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_timeScalar, outTime()), errorString);
        // file path and prim path affects on out stage data already done in base
        // class.
        AL_MAYA_CHECK_ERROR(
            attributeAffects(m_populationMaskIncludePaths, outStageData()), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_stageDataDirty, outStageData()), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_assetResolverConfig, outStageData()), errorString);
        AL_MAYA_CHECK_ERROR(attributeAffects(m_variantFallbacks, outStageData()), errorString);
    } catch (const MStatus& status) {
        return status;
    }

    addBaseTemplate("AEsurfaceShapeTemplate");
    generateAETemplate();

    return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onEditTargetChanged(
    UsdNotice::StageEditTargetChanged const& notice,
    UsdStageWeakPtr const&                   sender)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::onEditTargetChanged\n");
    if (!sender || sender != m_stage)
        return;

    trackEditTargetLayer();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::trackEditTargetLayer(LayerManager* layerManager)
{
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("ProxyShape::trackEditTargetLayer\n");
    auto stage = getUsdStage();

    if (!stage) {
        TF_DEBUG(ALUSDMAYA_LAYERS).Msg(" - no stage\n");
        return;
    }

    auto currTargetLayer = stage->GetEditTarget().GetLayer();

    TF_DEBUG(ALUSDMAYA_LAYERS)
        .Msg(" - curr target layer: %s\n", currTargetLayer->GetIdentifier().c_str());

    if (m_prevEditTarget != currTargetLayer) {
        if (!layerManager) {
            layerManager = LayerManager::findOrCreateManager();
            // findOrCreateManager SHOULD always return a result, but we check anyway,
            // to avoid any potential crash...
            if (!layerManager) {
                std::cerr << "Error creating / finding a layerManager node!" << std::endl;
                return;
            }
        }

        if (m_prevEditTarget && !m_prevEditTarget->IsDirty()) {
            // If the old edit target still isn't dirty, and we're switching to a new
            // edit target, we can remove it from the layer manager
            layerManager->removeLayer(m_prevEditTarget);
        }

        layerManager->addLayer(currTargetLayer);
        m_prevEditTarget = currTargetLayer;

        triggerEvent("EditTargetChanged");
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::trackAllDirtyLayers(LayerManager* layerManager)
{
    trackEditTargetLayer(layerManager);
    if (!layerManager)
        layerManager = LayerManager::findOrCreateManager();
    auto usedLayer = m_stage->GetUsedLayers();
    for (auto& _layer : usedLayer) {
        if (_layer->IsDirty())
            layerManager->addLayer(_layer);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onPrimResync(SdfPath primPath, SdfPathVector& previousPrims)
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("ProxyShape::onPrimResync checking %s\n", primPath.GetText());

    UsdPrim resyncPrim = m_stage->GetPrimAtPath(primPath);
    if (!resyncPrim.IsValid()) {
        return;
    }

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("ProxyShape::onPrimResync begin:\n%s\n", context()->serialise().asChar());

    AL_BEGIN_PROFILE_SECTION(ObjectChanged);
    MFnDagNode fn(thisMObject());
    MDagPath   proxyTransformPath;
    fn.getPath(proxyTransformPath);
    proxyTransformPath.pop();

    // find the new set of prims
    UsdPrimVector newPrimSet
        = huntForNativeNodesUnderPrim(proxyTransformPath, primPath, translatorManufacture());

    // Remove prims that have disappeared and translate in new prims
    translatePrimsIntoMaya(newPrimSet, previousPrims);

    previousPrims.clear();

    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
        .Msg("ProxyShape::onPrimResync end:\n%s\n", context()->serialise().asChar());

    AL_END_PROFILE_SECTION();

    validateTransforms();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::resync(const SdfPath& primPath)
{
    // FIMXE: This method was needed to call update() on all translators in the maya scene. Since
    // then some new locking and selectability functionality has been added to onObjectsChanged(). I
    // would want to call the logic in that method to handle this resyncing but it would need to be
    // refactored.

    SdfPathVector existingSchemaPrims;

    // populates list of prims from prim mapping that will change under the path to resync.
    onPrePrimChanged(primPath, existingSchemaPrims);

    onPrimResync(primPath, existingSchemaPrims);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialize(UsdStageRefPtr stage, LayerManager* layerManager)
{
    if (stage) {
        if (layerManager) {
            // Make sure the sessionLayer is always serialized (even if it's never an edit target)
            auto sessionLayer = stage->GetSessionLayer();
            layerManager->addLayer(sessionLayer);

            auto identifier = sessionLayer->GetIdentifier();
            if (layerManager->findLayer(identifier)) {
                // ...and store the name for the (anonymous) session layer so we can find it!
                sessionLayerNamePlug().setValue(AL::maya::utils::convert(identifier));
            } else {
                // ...make sure for the old Maya scene, we clear the sessionLayerName plug so there
                // is no complaint when we open it.
                sessionLayerNamePlug().setValue("");
            }

            auto rootLayer = stage->GetRootLayer();
            if (rootLayer->IsAnonymous()) {
                // For an anonymous root layer we need to update the file path to match the new
                // identifier the layer manager may have associated the layer with.
                MPlug             filePathPlug = this->filePathPlug();
                const std::string currentRootLayerId
                    = AL::maya::utils::convert(filePathPlug.asString());
                const std::string& newRootLayerId = rootLayer->GetIdentifier();
                if (currentRootLayerId != newRootLayerId) {
                    filePathPlug.setString(AL::maya::utils::convert(newRootLayerId));
                }
            }

            // Then add in the current edit target
            trackEditTargetLayer(layerManager);
        } else {
            MGlobal::displayError(
                "ProxyShape::serialize was passed a nullptr for the layerManager");
        }
        // Make sure our session layer is added to the layer manager to get it serialized.

        serialiseTranslatorContext();
        serialiseTransformRefs();
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serializeAll()
{
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg("ProxyShape::serializeAll\n");
    const char* errorString = "ProxyShape::serializeAll";
    // Now iterate over all proxyShapes...
    MFnDependencyNode fn;

    // Don't create a layerManager unless we find at least one proxy shape
    LayerManager* layerManager = nullptr;
    {
        MItDependencyNodes iter(MFn::kPluginShape);
        for (; !iter.isDone(); iter.next()) {
            MObject mobj = iter.item();
            fn.setObject(mobj);
            if (fn.typeId() != ProxyShape::kTypeId)
                continue;

            if (layerManager == nullptr) {
                layerManager = LayerManager::findOrCreateManager();
            }

            if (!layerManager) {
                MGlobal::displayError(MString("Error creating layerManager"));
                continue;
            }

            auto proxyShape = static_cast<ProxyShape*>(fn.userNode());
            if (proxyShape == nullptr) {
                MGlobal::displayError(MString("ProxyShape had no mpx data: ") + fn.name());
                continue;
            }

            UsdStageRefPtr stage = proxyShape->getUsdStage();

            if (!stage) {
                MGlobal::displayError(MString("Could not get stage for proxyShape: ") + fn.name());
                continue;
            }

            proxyShape->serialize(stage, layerManager);
        }

        // Bail if no proxyShapes were found...
        if (!layerManager)
            return;

        // Now that all layers are added, serialize to attributes
        AL_MAYA_CHECK_ERROR_RETURN(layerManager->populateSerialisationAttributes(), errorString);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onObjectsChanged(
    UsdNotice::ObjectsChanged const& notice,
    UsdStageWeakPtr const&           sender)
{
    if (MFileIO::isReadingFile()
        || AL::usdmaya::utils::BlockNotifications::isBlockingNotifications())
        return;

    if (!sender || sender != m_stage)
        return;

    TF_DEBUG(ALUSDMAYA_EVENTS)
        .Msg(
            "ProxyShape::onObjectsChanged called m_compositionHasChanged=%i\n",
            m_compositionHasChanged);

    if (!AL::usd::transaction::TransactionManager::InProgress(sender)) {
        TF_DEBUG(ALUSDMAYA_EVENTS)
            .Msg("ProxyShape::onObjectsChanged - no transaction in progress - processing all "
                 "changes\n");
        const UsdNotice::ObjectsChanged::PathRange resyncedPaths = notice.GetResyncedPaths();
        const UsdNotice::ObjectsChanged::PathRange changedOnlyPaths
            = notice.GetChangedInfoOnlyPaths();
        processChangedObjects(SdfPathVector(resyncedPaths), SdfPathVector(changedOnlyPaths));

        // If redraw wasn't requested from Maya i.e. external stage modification
        // We need to request redraw on idle, so viewport is updated
        if (!m_requestedRedraw) {
            m_requestedRedraw = true;
            MGlobal::executeCommandOnIdle("refresh");
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::validateTransforms()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("validateTransforms\n");
    if (m_stage) {
        SdfPathVector pathsToNuke;
        for (auto& it : m_requiredPaths) {
            TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("validateTransforms %s\n", it.first.GetText());

            MObject       node = it.second.node();
            MObjectHandle handle(node);
            if (!handle.isValid() || !handle.isAlive()) {
                continue;
            }

            if (node.isNull()) {
                continue;
            }

            Scope* tm = it.second.getTransformNode();
            if (!tm) {
                UsdPrim newPrim = m_stage->GetPrimAtPath(it.first);
                if (!newPrim) {
                    pathsToNuke.push_back(it.first);
                }
                continue;
            }

            UsdPrim newPrim = m_stage->GetPrimAtPath(it.first);
            if (newPrim) {
                std::string transformType;
                newPrim.GetMetadata(Metadata::transformType, &transformType);
                if (newPrim && transformType.empty()) {
                    tm->transform()->setPrim(newPrim, tm);
                }
            } else {
                pathsToNuke.push_back(it.first);
            }
        }
    }
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("/validateTransforms\n");
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onTransactionNotice(
    AL::usd::transaction::CloseNotice const& notice,
    const UsdStageWeakPtr&                   stage)
{
    TF_DEBUG(ALUSDMAYA_EVENTS)
        .Msg("ProxyShape::onTransactionNotice - transaction closed - processing changes\n");

    processChangedObjects(notice.GetResyncedPaths(), notice.GetChangedInfoOnlyPaths());
    if (!m_requestedRedraw) {
        m_requestedRedraw = true;
        MGlobal::executeCommandOnIdle("refresh");
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::processChangedObjects(
    const SdfPathVector& resyncedPaths,
    const SdfPathVector& changedOnlyPaths)
{
    TF_DEBUG(ALUSDMAYA_EVENTS).Msg("ProxyShape::processChangedObjects - processing changes\n");

    bool shouldCleanBBoxCache = false;

    if (!m_stage) {
        TF_DEBUG(ALUSDMAYA_EVENTS).Msg("ProxyShape::processChangedObjects - Invalid stage\n");
        return;
    }

    for (const SdfPath& path : resyncedPaths) {
        auto it = m_requiredPaths.find(path);
        if (it != m_requiredPaths.end()) {
            UsdPrim newPrim = m_stage->GetPrimAtPath(path);
            if (!newPrim.IsValid()) {
                TF_DEBUG(ALUSDMAYA_EVENTS)
                    .Msg(
                        "ProxyShape::processChangedObjects - resyncedPaths (1) contains invalid "
                        "path %s\n",
                        path.GetText());
                continue;
            }
            Scope* tm = it->second.getTransformNode();
            if (!tm)
                continue;
            BasicTransformationMatrix* tmm = tm->transform();
            if (!tmm)
                continue;
            tmm->setPrim(
                newPrim, tm); // Might be (invalid/nullptr) but that's OK at least it won't crash
        } else {
            UsdPrim newPrim = m_stage->GetPrimAtPath(path);
            if (!newPrim.IsValid()) {
                TF_DEBUG(ALUSDMAYA_EVENTS)
                    .Msg(
                        "ProxyShape::processChangedObjects - resyncedPaths (2) contains invalid "
                        "path %s\n",
                        path.GetText());
                continue;
            }
            if (newPrim && newPrim.IsA<UsdGeomXformable>()) {
                shouldCleanBBoxCache = true;
            }
        }
    }

    // check to see if any transform ops have been modified (update the bounds accordingly)
    if (!shouldCleanBBoxCache) {
        for (const SdfPath& path : changedOnlyPaths) {
            UsdPrim changedPrim = m_stage->GetPrimAtPath(path);
            if (path.IsPrimPropertyPath()) {
                const std::string tokenString = path.GetElementString();
                if (std::strncmp(tokenString.c_str(), ".xformOp", 8) == 0) {
                    shouldCleanBBoxCache = true;
                    break;
                }
            }
        }
    }

    // do we need to clear the bounding box cache?
    if (shouldCleanBBoxCache) {
        clearBoundingBoxCache();

        // Ideally we want to have a way to force maya to call ProxyShape::boundingBox() again to
        // update the bbox attributes. This may lead to a delay in the bbox updates (e.g. usually
        // you need to reselect the proxy before the bounds will be updated).
    }

    if (isLockPrimFeatureActive()) {
        processChangedMetaData(resyncedPaths, changedOnlyPaths);
    }

    // These paths are subtree-roots representing entire subtrees that may have
    // changed. In this case, we must dump all cached data below these points
    // and repopulate those trees.
    if (m_compositionHasChanged) {
        m_compositionHasChanged = false;
        onPrimResync(m_changedPath, m_variantSwitchedPrims);
        m_variantSwitchedPrims.clear();
        m_changedPath = SdfPath();

        std::stringstream strstr;
        strstr << "Breakdown for Variant Switch:\n";
        AL::usdmaya::Profiler::printReport(strstr);
    }
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<UsdPrim> ProxyShape::huntForNativeNodesUnderPrim(
    const MDagPath&                             proxyTransformPath,
    SdfPath                                     startPath,
    fileio::translators::TranslatorManufacture& manufacture,
    const bool                                  importAll)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::huntForNativeNodesUnderPrim\n");
    std::vector<UsdPrim>     prims;
    fileio::SchemaPrimsUtils utils(manufacture);

    const UsdPrim prim = m_stage->GetPrimAtPath(startPath);
    if (!prim.IsValid()) {
        MString errorString;
        errorString.format(
            MString("'^1s' is not a valid prim path in proxy shape: '^2s'"),
            startPath.GetString().c_str(),
            proxyTransformPath.fullPathName());
        MGlobal::displayError(errorString);
        return prims;
    }

    fileio::TransformIterator it(prim, proxyTransformPath);
    for (; !it.done(); it.next()) {
        UsdPrim prim = it.prim();
        if (!prim.IsValid()) {
            continue;
        }

        fileio::translators::TranslatorRefPtr trans = utils.isSchemaPrim(prim);
        if (trans && (trans->importableByDefault() || importAll)) {
            prims.push_back(prim);
        }
    }
    return prims;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onPrePrimChanged(const SdfPath& path, SdfPathVector& outPathVector)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::onPrePrimChanged\n");
    context()->preRemoveEntry(path, outPathVector);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::variantSelectionListener(SdfNotice::LayersDidChange const& notice)
// In order to detect changes to the variant selection we listen on the SdfNotice::LayersDidChange
// global notice which is sent to indicate that layer contents have changed.  We are then able to
// access the change list to check if a variant selection change happened.  If so, we trigger a
// ProxyShapePostLoadProcess() which will regenerate the alTransform nodes based on the contents of
// the new variant selection.
{
    if (MFileIO::isReadingFile())
        return;

    if (!m_stage)
        return;

    const SdfLayerHandleVector stack = m_stage->GetLayerStack();

    TF_FOR_ALL(itr, notice.GetChangeListVec())
    {
        if (std::find(stack.begin(), stack.end(), itr->first) == stack.end())
            continue;

        TF_FOR_ALL(entryIter, itr->second.GetEntryList())
        {
            const SdfPath&              path = entryIter->first;
            const SdfChangeList::Entry& entry = entryIter->second;

            TF_FOR_ALL(it, entry.infoChanged)
            {
                if (it->first == SdfFieldKeys->VariantSelection
                    || it->first == SdfFieldKeys->Active) {
                    triggerEvent("PreVariantChangedCB");

                    TF_DEBUG(ALUSDMAYA_EVENTS)
                        .Msg(
                            "ProxyShape::variantSelectionListener oldPath=%s, oldIdentifier=%s, "
                            "path=%s, layer=%s\n",
                            entry.oldPath.GetText(),
                            entry.oldIdentifier.c_str(),
                            path.GetText(),
                            itr->first->GetIdentifier().c_str());
                    if (!m_compositionHasChanged) {
                        TF_DEBUG(ALUSDMAYA_EVALUATION)
                            .Msg("ProxyShape::Not yet in a composition change state. Recording "
                                 "path. \n");
                        m_changedPath = path;
                    }
                    m_compositionHasChanged = true;
                    onPrePrimChanged(path, m_variantSwitchedPrims);

                    triggerEvent("PostVariantChangedCB");
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::loadStage()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::loadStage\n");

    triggerEvent("PreStageLoaded");

    AL_BEGIN_PROFILE_SECTION(LoadStage);
    MDataBlock dataBlock = forceCache();

    const int         stageIdVal = inputInt32Value(dataBlock, stageCacheId());
    UsdStageCache::Id stageId = UsdStageCache::Id().FromLongInt(stageIdVal);
    MString           file = inputStringValue(dataBlock, filePath());

    if (m_stage) {
        // In case there was already a stage in m_stage, check to see if it's edit target has been
        // altered.
        trackEditTargetLayer();

        if (StageCache::Get().Contains(stageId)) {
            auto          stageFromId = StageCache::Get().Find(stageId);
            const MString stageFilePath
                = AL::maya::utils::convert(stageFromId->GetRootLayer()->GetIdentifier());
            if (file != stageFilePath) {
                // When we have an existing stage and a valid stageCacheId, if file paths of the
                // stage from cache doesn't match that the one we are holding on to, then looks like
                // file path was changed. Drop the current cache Id.
                stageId = UsdStageCache::Id();
            }
        }
    }

    if (stageId.IsValid()) {
        // Load stage from cache.
        if (StageCache::Get().Contains(stageId)) {
            m_stage = StageCache::Get().Find(stageId);
            // Save the initial edit target and all dirty layers.
            trackAllDirtyLayers();
            file.set(m_stage->GetRootLayer()->GetIdentifier().c_str());
            outputStringValue(dataBlock, filePath(), file);
        } else {
            MGlobal::displayError(
                MString("ProxyShape::loadStage called with non-existent stageCacheId ")
                + stageId.ToString().c_str());
            stageId = UsdStageCache::Id();
        }

        // Save variant fallbacks from session layer to Maya node attribute
        if (m_stage) {
            saveVariantFallbacks(
                getVariantFallbacksFromLayer(m_stage->GetSessionLayer()), dataBlock);
        }
    } else {
        m_stage = UsdStageRefPtr();

        // Get input attr values
        const MString sessionLayerName = inputStringValue(dataBlock, m_sessionLayerName);

        const MString populationMaskIncludePaths
            = inputStringValue(dataBlock, m_populationMaskIncludePaths);
        UsdStagePopulationMask mask = constructStagePopulationMask(populationMaskIncludePaths);

        // TODO initialise the context using the serialised attribute

        // let the usd stage cache deal with caching the usd stage data
        std::string fileString = TfStringTrimRight(file.asChar());

        TF_DEBUG(ALUSDMAYA_TRANSLATORS)
            .Msg("ProxyShape::reloadStage original USD file path is %s\n", fileString.c_str());

        SdfLayerRefPtr rootLayer;
        if (SdfLayer::IsAnonymousLayerIdentifier(fileString)) {
            // For anonymous root layer we must explicitly ask for from the layer manager.
            // This is because USD does not allow us to create a new anonymous SdfLayer
            // with the exact same identifier. The best we can do is to ask the layer manager
            // to create the anonymous layer, and let it manage the identifier mappings.
            if (auto layerManager = LayerManager::findManager()) {
                rootLayer = layerManager->findLayer(fileString);
            }

            if (rootLayer) {
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShape::reloadStage found anonymous layer %s from layer manager\n",
                        fileString.c_str());
            } else {
                const std::string tag = SdfLayer::GetDisplayNameFromIdentifier(fileString);
                rootLayer = SdfLayer::CreateAnonymous(tag);
                if (rootLayer) {
                    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                        .Msg(
                            "ProxyShape::reloadStage created anonymous layer %s (renamed to %s)\n",
                            fileString.c_str(),
                            rootLayer->GetIdentifier().c_str());
                }
            }
        } else {
            boost::filesystem::path filestringPath(fileString);
            if (filestringPath.is_absolute()) {
                fileString = UsdMayaUtilFileSystem::resolvePath(fileString);
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShape::reloadStage resolved the USD file path to %s\n",
                        fileString.c_str());
            } else {
                fileString = UsdMayaUtilFileSystem::resolveRelativePathWithinMayaContext(
                    thisMObject(), fileString);
                TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                    .Msg(
                        "ProxyShape::reloadStage resolved the relative USD file path to %s\n",
                        fileString.c_str());
            }

            // Fall back on providing the path "as is" to USD
            if (fileString.empty()) {
                fileString.assign(file.asChar(), file.length());
            }

            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg("ProxyShape::loadStage called for the usd file: %s\n", fileString.c_str());
            rootLayer = SdfLayer::FindOrOpen(fileString);
        }

        // Only try to create a stage for layers that can be opened.
        if (rootLayer) {
            MStatus        status;
            SdfLayerRefPtr sessionLayer;

            AL_BEGIN_PROFILE_SECTION(OpeningUsdStage);
            AL_BEGIN_PROFILE_SECTION(OpeningSessionLayer);
            {
                // Grab the session layer from the layer manager
                if (sessionLayerName.length() > 0) {
                    auto layerManager = LayerManager::findManager();
                    if (layerManager) {
                        sessionLayer
                            = layerManager->findLayer(AL::maya::utils::convert(sessionLayerName));
                        if (!sessionLayer) {
                            MGlobal::displayError(
                                MString("ProxyShape \"") + name()
                                + "\" had a serialized session layer"
                                  " named \""
                                + sessionLayerName
                                + "\", but no matching layer could be found in the layerManager");
                        }
                    } else {
                        MGlobal::displayError(
                            MString("ProxyShape \"") + name()
                            + "\" had a serialized session layer,"
                              " but no layerManager node was found");
                    }
                }

                // If we still have no sessionLayer, but there's data in serializedSessionLayer,
                // then assume we're reading an "old" file, and read it for backwards compatibility.
                if (!sessionLayer) {
                    const MString serializedSessionLayer
                        = inputStringValue(dataBlock, m_serializedSessionLayer);
                    if (serializedSessionLayer.length() != 0) {
                        sessionLayer = SdfLayer::CreateAnonymous();
                        sessionLayer->ImportFromString(
                            AL::maya::utils::convert(serializedSessionLayer));
                    }
                }
            }
            AL_END_PROFILE_SECTION();

            AL_BEGIN_PROFILE_SECTION(OpenRootLayer);

            const MString assetResolverConfig = inputStringValue(dataBlock, m_assetResolverConfig);

            if (assetResolverConfig.length() == 0) {
                // Initialise the asset resolver with the filepath
                PXR_NS::ArGetResolver().ConfigureResolverForAsset(fileString);
            } else {
                // Initialise the asset resolver with the resolverConfig string
                PXR_NS::ArGetResolver().ConfigureResolverForAsset(assetResolverConfig.asChar());
            }
            AL_END_PROFILE_SECTION();

            AL_BEGIN_PROFILE_SECTION(UpdateGlobalVariantFallbacks);
            PcpVariantFallbackMap defaultVariantFallbacks;
            PcpVariantFallbackMap fallbacks(
                updateVariantFallbacks(defaultVariantFallbacks, dataBlock));
            AL_END_PROFILE_SECTION();

            AL_BEGIN_PROFILE_SECTION(UsdStageOpen);
            {
                UsdStageCacheContext ctx(StageCache::Get());

                bool                     unloadedFlag = inputBoolValue(dataBlock, m_unloaded);
                UsdStage::InitialLoadSet loadOperation
                    = unloadedFlag ? UsdStage::LoadNone : UsdStage::LoadAll;

                if (sessionLayer) {
                    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                        .Msg("ProxyShape::loadStage is called with extra session layer.\n");
                    m_stage = UsdStage::OpenMasked(rootLayer, sessionLayer, mask, loadOperation);
                } else {
                    TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                        .Msg("ProxyShape::loadStage is called without any session layer.\n");
                    m_stage = UsdStage::OpenMasked(rootLayer, mask, loadOperation);
                }

                // Expand the mask, since we do not really want to mask the possible relation
                // targets.
                m_stage->ExpandPopulationMask();

                stageId = StageCache::Get().Insert(m_stage);
                outputInt32Value(dataBlock, stageCacheId(), stageId.ToLongInt());

                // Set the stage in datablock so it's ready in case it needs to be accessed
                MObject           data;
                MayaUsdStageData* usdStageData
                    = createData<MayaUsdStageData>(MayaUsdStageData::mayaTypeId, data);
                usdStageData->stage = m_stage;
                usdStageData->primPath = m_path;
                outputDataValue(dataBlock, outStageData(), usdStageData);

                // Set the edit target to the session layer so any user interaction will wind up
                // there
                m_stage->SetEditTarget(m_stage->GetSessionLayer());
                // Save the initial edit target
                trackEditTargetLayer();
            }
            AL_END_PROFILE_SECTION();

            AL_BEGIN_PROFILE_SECTION(ResetGlobalVariantFallbacks);
            // reset only if the global variant fallbacks has been modified
            if (!fallbacks.empty()) {
                saveVariantFallbacks(convertVariantFallbacksToStr(fallbacks), dataBlock);
                // restore default value
                UsdStage::SetGlobalVariantFallbacks(defaultVariantFallbacks);
            }
            AL_END_PROFILE_SECTION();

            AL_END_PROFILE_SECTION();
        } else if (!fileString.empty()) {
            TF_DEBUG(ALUSDMAYA_TRANSLATORS)
                .Msg("ProxyShape::loadStage failed to open the usd file: %s.\n", file.asChar());
            MGlobal::displayWarning(MString("Failed to open usd file \"") + file + "\"");
        }
    }

    // Get the prim
    // If no primPath string specified, then use the pseudo-root.
    const SdfPath rootPath(std::string("/"));
    MString       primPathStr = inputStringValue(dataBlock, primPath());
    if (primPathStr.length()) {
        m_path = SdfPath(AL::maya::utils::convert(primPathStr));
        UsdPrim prim = m_stage->GetPrimAtPath(m_path);
        if (!prim) {
            m_path = rootPath;
        }
    } else {
        m_path = rootPath;
    }

    if (m_stage && !MFileIO::isReadingFile()) {
        AL_BEGIN_PROFILE_SECTION(PostLoadProcess);
        // execute the post load process to import any custom prims
        cmds::ProxyShapePostLoadProcess::initialise(this);
        if (isLockPrimFeatureActive()) {
            findPrimsWithMetaData();
        }
        AL_END_PROFILE_SECTION();
    }

    AL_END_PROFILE_SECTION();

    if (MGlobal::kInteractive == MGlobal::mayaState()) {
        std::stringstream strstr;
        strstr << "Breakdown for file: " << file << std::endl;
        AL::usdmaya::Profiler::printReport(strstr);
        MGlobal::displayInfo(AL::maya::utils::convert(strstr.str()));
    }

    destroyGLImagingEngine();
    stageDataDirtyPlug().setValue(true);

    triggerEvent("PostStageLoaded");
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::postConstructor()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::postConstructor\n");

    ParentClass::postConstructor();

    // Apply render defaults
    MPlug(thisMObject(), m_visibleInReflections).setValue(true);
    MPlug(thisMObject(), m_visibleInRefractions).setValue(true);
}

//----------------------------------------------------------------------------------------------------------------------
MString ProxyShape::recordUsdPrimToMayaPath(const UsdPrim& usdPrim, const MObject& mayaObject)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION)
        .Msg(
            "ProxyShape::recordUsdPrimToMayaPath store path to %s\n",
            usdPrim.GetPrimPath().GetText());

    // Retrieve the proxy shapes transform path which will be used in the
    // UsdPrim->MayaNode mapping in the case where there is delayed node creation.
    MFnDagNode    shapeFn(thisMObject());
    const MObject shapeParent = shapeFn.parent(0);
    MDagPath      mayaPath;
    // Note: This doesn't account for the possibility of multiple paths to a node, but so far this
    // is only used for recently created transforms that should only have single paths.
    MDagPath::getAPathTo(shapeParent, mayaPath);

    MString resultingPath;
    SdfPath primPath(usdPrim.GetPath());
    resultingPath = AL::usdmaya::utils::mapUsdPrimToMayaNode(usdPrim, mayaObject, &mayaPath);
    m_primPathToDagPath.emplace(primPath, resultingPath);

    return resultingPath;
}

//----------------------------------------------------------------------------------------------------------------------
MString ProxyShape::getMayaPathFromUsdPrim(const UsdPrim& usdPrim) const
{
    PrimPathToDagPath::const_iterator itr = m_primPathToDagPath.find(usdPrim.GetPath());
    if (itr == m_primPathToDagPath.end()) {
        TF_DEBUG(ALUSDMAYA_EVALUATION)
            .Msg("ProxyShape::getMayaPathFromUsdPrim could not find stored MayaPath\n");
        return MString();
    }
    return itr->second;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::copyInternalData(MPxNode* srcNode)
{
    // On duplication, the ProxyShape has a null stage, and m_filePathDirty is
    // false, even if the file path attribute is set.  We must ensure the next
    // call to computeOutStageData() calls loadStage().
    m_filePathDirty = true;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::computeOutStageData(const MPlug& plug, MDataBlock& dataBlock)
{
    // create new stage data
    MObject           data;
    MayaUsdStageData* usdStageData
        = createData<MayaUsdStageData>(MayaUsdStageData::mayaTypeId, data);
    if (!usdStageData) {
        return MS::kFailure;
    }

    // make sure a stage is loaded
    if (!m_stage && m_filePathDirty) {
        m_filePathDirty = false;
        loadStage();
    }
    // Set the output stage data params
    usdStageData->stage = m_stage;
    usdStageData->primPath = m_path;

    // set the cached output value, and flush
    MStatus status = outputDataValue(dataBlock, outStageData(), usdStageData);
    if (!status) {
        return MS::kFailure;
    }

    MayaUsdProxyStageSetNotice(*this).Send();

    return status;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::isStageValid() const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::isStageValid\n");
    MDataBlock dataBlock = const_cast<ProxyShape*>(this)->forceCache();

    MayaUsdStageData* outData = inputDataValue<MayaUsdStageData>(dataBlock, outStageData());
    if (outData && outData->stage)
        return true;

    return false;
}

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr ProxyShape::getUsdStage() const
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getUsdStage\n");

    MPlug   plug(thisMObject(), outStageData());
    MObject data;
    plug.getValue(data);
    MFnPluginData     fnData(data);
    MayaUsdStageData* outData = static_cast<MayaUsdStageData*>(fnData.data());
    if (outData) {
        return outData->stage;
    }
    return UsdStageRefPtr();
}

UsdTimeCode ProxyShape::getTime() const
{
    return UsdTimeCode(outTimePlug().asMTime().as(MTime::uiUnit()));
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::computeOutputTime(const MPlug& plug, MDataBlock& dataBlock, MTime& currentTime)
{
    MTime  inTime = inputTimeValue(dataBlock, time());
    MTime  inTimeOffset = inputTimeValue(dataBlock, m_timeOffset);
    double inTimeScalar = inputDoubleValue(dataBlock, m_timeScalar);
    currentTime.setValue(
        (inTime.as(MTime::uiUnit()) - inTimeOffset.as(MTime::uiUnit())) * inTimeScalar);
    return outputTimeValue(dataBlock, outTime(), currentTime);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::compute(const MPlug& plug, MDataBlock& dataBlock)
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::compute %s\n", plug.name().asChar());
    // When shape is computed Maya will request redraw by itself
    m_requestedRedraw = true;
    MTime currentTime;
    if (plug == outTime()) {
        return computeOutputTime(plug, dataBlock, currentTime);
    } else if (plug == outStageData()) {
        MStatus status = computeOutputTime(MPlug(plug.node(), outTime()), dataBlock, currentTime);
        return status == MS::kSuccess ? computeOutStageData(plug, dataBlock) : status;
    }
    // Completely skip over parent class compute(), because it has inStageData
    // and inStageDataCached attributes we don't use.
    return MPxSurfaceShape::compute(plug, dataBlock);
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::setInternalValue(const MPlug& plug, const MDataHandle& dataHandle)
{
    // We set the value in the datablock ourselves, so that the plug's value is
    // can be queried from subfunctions (ie, loadStage, constructExcludedPrims, etc)
    //
    // If we simply returned "false", the "standard" implementation would set
    // the datablock for us, but this would be too late for these subfunctions
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::setInternalValue %s\n", plug.name().asChar());

    if (plug == filePath() || plug == m_assetResolverConfig || plug == stageCacheId()
        || plug == m_variantFallbacks) {
        m_filePathDirty = true;

        // can't use dataHandle.datablock(), as this is a temporary datahandle
        MDataBlock datablock = forceCache();

        if (plug == filePath() || plug == m_assetResolverConfig || plug == m_variantFallbacks) {
            AL_MAYA_CHECK_ERROR_RETURN_VAL(
                outputStringValue(datablock, plug, dataHandle.asString()),
                false,
                "ProxyShape::setInternalValue - error setting filePath, assetResolverConfig or "
                "variantFallbacks");
        } else {
            AL_MAYA_CHECK_ERROR_RETURN_VAL(
                outputInt32Value(datablock, plug, dataHandle.asInt()),
                false,
                "ProxyShape::setInternalValue - error setting stageCacheId");
        }
        // Delay stage creation if opening a file, because we haven't created the LayerManager node
        // yet
        if (MFileIO::isReadingFile()) {
            m_unloadedProxyShapes.push_back(MObjectHandle(thisMObject()));
        } else {
            loadStage();
        }
        return true;
    } else if (plug == primPath()) {
        // can't use dataHandle.datablock(), as this is a temporary datahandle
        MDataBlock datablock = forceCache();
        AL_MAYA_CHECK_ERROR_RETURN_VAL(
            outputStringValue(datablock, primPath(), dataHandle.asString()),
            false,
            "ProxyShape::setInternalValue - error setting primPath");

        if (m_stage) {
            // Get the prim
            // If no primPath string specified, then use the pseudo-root.
            MString primPathStr = dataHandle.asString();
            if (primPathStr.length()) {
                m_path = SdfPath(AL::maya::utils::convert(primPathStr));
                UsdPrim prim = m_stage->GetPrimAtPath(m_path);
                if (!prim) {
                    m_path = SdfPath::AbsoluteRootPath();
                }
            } else {
                m_path = SdfPath::AbsoluteRootPath();
            }
            constructGLImagingEngine();
        }
        return true;
    } else if (plug == excludePrimPaths() || plug == m_excludedTranslatedGeometry) {
        // can't use dataHandle.datablock(), as this is a temporary datahandle
        MDataBlock datablock = forceCache();
        AL_MAYA_CHECK_ERROR_RETURN_VAL(
            outputStringValue(datablock, plug.attribute(), dataHandle.asString()),
            false,
            MString("ProxyShape::setInternalValue - error setting ") + plug.name());

        if (m_stage) {
            constructExcludedPrims();
        }
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::getInternalValue(const MPlug& plug, MDataHandle& dataHandle)
{
    // Not sure if this is needed... don't know behavior of default implementation?
    return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::isBounded() const { return true; }

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::CacheEmptyBoundingBox(MBoundingBox& cachedBBox)
{
    cachedBBox = MBoundingBox(
        MPoint(-100000.0f, -100000.0f, -100000.0f), MPoint(100000.0f, 100000.0f, 100000.0f));
}

//----------------------------------------------------------------------------------------------------------------------
UsdTimeCode ProxyShape::GetOutputTime(MDataBlock dataBlock) const
{
    return UsdTimeCode(inputDoubleValue(dataBlock, outTime()));
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::unloadMayaReferences()
{
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::unloadMayaReferences called\n");
    MObjectArray references;
    for (auto it = m_requiredPaths.begin(), e = m_requiredPaths.end(); it != e; ++it) {
        MStatus           status;
        MFnDependencyNode fn(it->second.node(), &status);
        if (status) {
            MPlug plug = fn.findPlug("message", &status);
            if (status) {
                MPlugArray plugs;
                plug.connectedTo(plugs, false, true);
                for (uint32_t i = 0; i < plugs.length(); ++i) {
                    MObject temp = plugs[i].node();
                    if (temp.hasFn(MFn::kReference)) {
                        MFnReference mfnRef(temp);
                        MString      referenceFilename = mfnRef.fileName(
                            true /*resolvedName*/,
                            true /*includePath*/,
                            true /*includeCopyNumber*/);
                        TF_DEBUG(ALUSDMAYA_EVALUATION)
                            .Msg(
                                "ProxyShape::unloadMayaReferences unloading %s\n",
                                referenceFilename.asChar());
                        MFileIO::unloadReferenceByNode(temp);
                    }
                }
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialiseTransformRefs()
{
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::serialiseTransformRefs\n");
    triggerEvent("PreSerialiseTransformRefs");

    std::ostringstream oss;
    for (auto iter : m_requiredPaths) {
        MStatus       status;
        MObjectHandle handle(iter.second.node());

        if (handle.isAlive() && handle.isValid()) {
            MFnDagNode fn(handle.object(), &status);
            if (status) {
                MDagPath path;
                fn.getPath(path);
                oss << path.fullPathName() << " " << iter.first.GetText() << " "
                    << uint32_t(iter.second.required()) << " " << uint32_t(iter.second.selected())
                    << " " << uint32_t(iter.second.refCount()) << ";";
            }
        }
    }
    serializedRefCountsPlug().setString(oss.str().c_str());

    triggerEvent("PostSerialiseTransformRefs");
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::deserialiseTransformRefs()
{
    triggerEvent("PreDeserialiseTransformRefs");

    MString      str = serializedRefCountsPlug().asString();
    MStringArray strs;
    str.split(';', strs);

    for (uint32_t i = 0, n = strs.length(); i < n; ++i) {
        if (strs[i].length()) {
            MStringArray tstrs;
            strs[i].split(' ', tstrs);
            MString nodeName = tstrs[0];

            MSelectionList sl;
            if (sl.add(nodeName)) {
                MObject node;
                if (sl.getDependNode(0, node)) {
                    MFnDependencyNode fn(node);
                    Scope*            transformNode = dynamic_cast<Scope*>(fn.userNode());
                    if (transformNode) {
                        const uint32_t required = tstrs[2].asUnsigned();
                        const uint32_t selected = tstrs[3].asUnsigned();
                        const uint32_t refCounts = tstrs[4].asUnsigned();
                        SdfPath        path(tstrs[1].asChar());
                        m_requiredPaths.emplace(
                            path,
                            TransformReference(node, transformNode, required, selected, refCounts));
                        TF_DEBUG(ALUSDMAYA_EVALUATION)
                            .Msg(
                                "ProxyShape::deserialiseTransformRefs m_requiredPaths added "
                                "AL_usdmaya_Transform TransformReference: %s\n",
                                path.GetText());
                    } else {
                        const uint32_t required = tstrs[2].asUnsigned();
                        const uint32_t selected = tstrs[3].asUnsigned();
                        const uint32_t refCounts = tstrs[4].asUnsigned();
                        SdfPath        path(tstrs[1].asChar());
                        m_requiredPaths.emplace(
                            path, TransformReference(node, nullptr, required, selected, refCounts));
                        TF_DEBUG(ALUSDMAYA_EVALUATION)
                            .Msg(
                                "ProxyShape::deserialiseTransformRefs m_requiredPaths added "
                                "TransformReference: %s\n",
                                path.GetText());
                    }
                }
            }
        }
    }

    serializedRefCountsPlug().setString("");

    triggerEvent("PostDeserialiseTransformRefs");
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::TransformReference::TransformReference(
    MObject  mayaNode,
    Scope*   node,
    uint32_t r,
    uint32_t s,
    uint32_t rc)
    : m_node(mayaNode)
    , m_transform(nullptr)
{
    m_required = r;
    m_selected = s;
    m_selectedTemp = 0;
    m_refCount = rc;
    m_transform = getTransformNode();
}

//----------------------------------------------------------------------------------------------------------------------
Scope* ProxyShape::TransformReference::getTransformNode() const
{
    MObjectHandle n(node());
    if (n.isValid() && n.isAlive()) {
        MStatus           status;
        MFnDependencyNode fn(n.object(), &status);
        if (status == MS::kSuccess) {
            Scope* transformNode = dynamic_cast<Scope*>(fn.userNode());
            if (transformNode) {
                TF_DEBUG(ALUSDMAYA_EVALUATION)
                    .Msg(
                        "TransformReference::transform found valid AL_usdmaya Transform or Scope: "
                        "%s\n",
                        fn.absoluteName().asChar());
                return (Scope*)fn.userNode();
            } else {
                TF_DEBUG(ALUSDMAYA_EVALUATION)
                    .Msg(
                        "TransformReference::transform found non AL_usdmaya_Tranform: %s\n",
                        fn.absoluteName().asChar());
                return nullptr;
            }
        } else {
            TF_DEBUG(ALUSDMAYA_EVALUATION)
                .Msg("TransformReference::transform found invalid transform\n");
            return nullptr;
        }
    }
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformReference::transform found null transform\n");
    return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::cleanupTransformRefs()
{
    for (auto it = m_requiredPaths.begin(); it != m_requiredPaths.end();) {
        if (!it->second.selected() && !it->second.required() && !it->second.refCount()) {
            TF_DEBUG(ALUSDMAYA_EVALUATION)
                .Msg(
                    "ProxyShape::cleanupTransformRefs m_requiredPaths removed TransformReference: "
                    "%s\n",
                    it->first.GetText());
            m_requiredPaths.erase(it++);
        } else {
            ++it;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::registerEvents()
{
    registerEvent("PreDestroyProxyShape", AL::event::kUSDMayaEventType);
    registerEvent("PostDestroyProxyShape", AL::event::kUSDMayaEventType);
    registerEvent("PreStageLoaded", AL::event::kUSDMayaEventType);
    registerEvent("PostStageLoaded", AL::event::kUSDMayaEventType);
    registerEvent("ConstructGLEngine", AL::event::kUSDMayaEventType);
    registerEvent("DestroyGLEngine", AL::event::kUSDMayaEventType);
    registerEvent("PreSelectionChanged", AL::event::kUSDMayaEventType);
    registerEvent("PostSelectionChanged", AL::event::kUSDMayaEventType);
    registerEvent("PreVariantChanged", AL::event::kUSDMayaEventType);
    registerEvent("PostVariantChanged", AL::event::kUSDMayaEventType);
    registerEvent("PreSerialiseContext", AL::event::kUSDMayaEventType, Global::postSave());
    registerEvent("PostSerialiseContext", AL::event::kUSDMayaEventType, Global::postSave());
    registerEvent("PreDeserialiseContext", AL::event::kUSDMayaEventType, Global::postRead());
    registerEvent("PostDeserialiseContext", AL::event::kUSDMayaEventType, Global::postRead());
    registerEvent("PreSerialiseTransformRefs", AL::event::kUSDMayaEventType, Global::postSave());
    registerEvent("PostSerialiseTransformRefs", AL::event::kUSDMayaEventType, Global::postSave());
    registerEvent("PreDeserialiseTransformRefs", AL::event::kUSDMayaEventType, Global::postRead());
    registerEvent("PostDeserialiseTransformRefs", AL::event::kUSDMayaEventType, Global::postRead());
    registerEvent("EditTargetChanged", AL::event::kUSDMayaEventType);
    registerEvent("SelectionStarted", AL::event::kUSDMayaEventType);
    registerEvent("SelectionEnded", AL::event::kUSDMayaEventType);
}

//----------------------------------------------------------------------------------------------------------------------
MSelectionMask ProxyShape::getShapeSelectionMask() const
{
    MSelectionMask::SelectionType selType = MSelectionMask::kSelectMeshes;
    return MSelectionMask(selType);
}

//----------------------------------------------------------------------------------------------------------------------
} // namespace nodes
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
