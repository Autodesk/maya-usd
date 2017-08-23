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
#include "pxr/usdImaging/usdImaging/version.h"
#if (USD_IMAGING_API_VERSION >= 7)
  #include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#else
  #include "pxr/usdImaging/usdImaging/hdEngine.h"
#endif

#include "AL/maya/CodeTimings.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/StageData.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/Utils.h"
#include "AL/usdmaya/cmds/ProxyShapePostLoadProcess.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Layer.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"

#include "maya/MFileIO.h"
#include "maya/MFnPluginData.h"
#include "maya/MHWGeometryUtilities.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MPlugArray.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stageCacheContext.h"


namespace AL {
namespace usdmaya {
namespace nodes {

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialiseTranslatorContext()
{
  serializedTrCtxPlug().setValue(context()->serialise());
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::deserialiseTranslatorContext()
{
  MString value;
  serializedTrCtxPlug().getValue(value);
  context()->deserialise(value);
}

//----------------------------------------------------------------------------------------------------------------------
static std::string resolvePath(const std::string& filePath)
{
  ArResolver& resolver = ArGetResolver();

  return resolver.Resolve(filePath);
}

//----------------------------------------------------------------------------------------------------------------------
static void beforeSaveScene(void* clientData)
{
  ProxyShape* proxyShape =  static_cast<ProxyShape *>(clientData);
  UsdStageRefPtr stage = proxyShape->getUsdStage();

  if(stage)
  {
    std::string serializeSessionLayerStr;
    stage->GetSessionLayer()->ExportToString(&serializeSessionLayerStr);

    MPlug serializeSessionLayerPlug(proxyShape->thisMObject(), proxyShape->serializedSessionLayer());
    serializeSessionLayerPlug.setValue(convert(serializeSessionLayerStr));

    proxyShape->serialiseTranslatorContext();
    proxyShape->serialiseTransformRefs();

    // prior to saving, serialize any modified layers
    MFnDependencyNode fn;
    MItDependencyNodes iter(MFn::kPluginDependNode);
    for(; !iter.isDone(); iter.next())
    {
      fn.setObject(iter.item());
      if(fn.typeId() == Layer::kTypeId)
      {
        TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("serialising layer: %s\n", fn.name().asChar());
        Layer* layerPtr = (Layer*)fn.userNode();
        layerPtr->populateSerialisationAttributes();
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(ProxyShape, AL_USDMAYA_PROXYSHAPE, AL_usdmaya);

MObject ProxyShape::m_filePath = MObject::kNullObj;
MObject ProxyShape::m_primPath = MObject::kNullObj;
MObject ProxyShape::m_excludePrimPaths = MObject::kNullObj;
MObject ProxyShape::m_time = MObject::kNullObj;
MObject ProxyShape::m_timeOffset = MObject::kNullObj;
MObject ProxyShape::m_timeScalar = MObject::kNullObj;
MObject ProxyShape::m_outTime = MObject::kNullObj;
MObject ProxyShape::m_complexity = MObject::kNullObj;
MObject ProxyShape::m_outStageData = MObject::kNullObj;
MObject ProxyShape::m_displayGuides = MObject::kNullObj;
MObject ProxyShape::m_displayRenderGuides = MObject::kNullObj;
MObject ProxyShape::m_layers = MObject::kNullObj;
MObject ProxyShape::m_serializedSessionLayer = MObject::kNullObj;
MObject ProxyShape::m_serializedArCtx = MObject::kNullObj;
MObject ProxyShape::m_serializedTrCtx = MObject::kNullObj;
MObject ProxyShape::m_unloaded = MObject::kNullObj;
MObject ProxyShape::m_inDrivenTransformsData = MObject::kNullObj;
MObject ProxyShape::m_ambient = MObject::kNullObj;
MObject ProxyShape::m_diffuse = MObject::kNullObj;
MObject ProxyShape::m_specular = MObject::kNullObj;
MObject ProxyShape::m_emission = MObject::kNullObj;
MObject ProxyShape::m_shininess = MObject::kNullObj;
MObject ProxyShape::m_serializedRefCounts = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
Layer* ProxyShape::getLayer()
{
  MPlug plug(thisMObject(), m_layers);
  MFnDependencyNode fn;

  MPlugArray plugs;
  if(plug.connectedTo(plugs, true, true))
  {
    if(plugs.length())
    {
      if(plugs[0].node().apiType() == MFn::kPluginDependNode)
      {
        if(fn.setObject(plugs[0].node()))
        {
          if(fn.typeId() == Layer::kTypeId)
          {
            return (Layer*)fn.userNode();
          }
          else
          {
            MGlobal::displayError(MString("Invalid connection found on attribute") + plug.name());
          }
        }
        else
        {
          MGlobal::displayError(MString("Invalid connection found on attribute") + plug.name());
        }
      }
      else
      {
        MGlobal::displayError(MString("Invalid connection found on attribute") + plug.name());
      }
    }
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
Layer* ProxyShape::findLayer(SdfLayerHandle handle)
{
  LAYER_HANDLE_CHECK(handle);
  if(handle)
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::findLayer: %s\n", handle->GetIdentifier().c_str());
    Layer* layer = getLayer();
    if(layer)
    {
      return layer->findLayer(handle);
    }
  }
  // we shouldn't really be able to get here!
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------
MString ProxyShape::findLayerMayaName(SdfLayerHandle handle)
{
  LAYER_HANDLE_CHECK(handle);
  if(handle)
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::findLayerMayaName: %s\n", handle->GetIdentifier().c_str());
    Layer* node = findLayer(handle);
    if(node)
    {
      MFnDependencyNode fn(node->thisMObject());
      return fn.name();
    }
  }
  return MString();
}

//----------------------------------------------------------------------------------------------------------------------
UsdPrim ProxyShape::getUsdPrim(MDataBlock& dataBlock) const
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getUsdPrim\n");
  UsdPrim usdPrim;
  StageData* outData = inputDataValue<StageData>(dataBlock, m_outStageData);
  if(outData)
  {
    if(outData->stage)
    {
      usdPrim = (outData->primPath.IsEmpty()) ?
                 outData->stage->GetPseudoRoot() :
                 outData->stage->GetPrimAtPath(outData->primPath);
    }
  }
  return usdPrim;
}

//----------------------------------------------------------------------------------------------------------------------
SdfPathVector ProxyShape::getExcludePrimPaths() const
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getExcludePrimPaths\n");

  SdfPathVector result;

  MString paths = excludePrimPathsPlug().asString();
  if(paths.length())
  {
    const char* begin = paths.asChar();
    const char* end = paths.asChar() + paths.length();
    const char* iter = std::find(begin, end, ',');
    while(iter != end)
    {
      result.push_back(SdfPath(std::string(begin, iter)));
      begin = iter + 1;
      iter = std::find(begin, end, ',');
    }
    result.push_back(SdfPath(std::string(begin, end)));
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructGLImagingEngine()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::constructGLImagingEngine\n");
  if (MGlobal::mayaState() != MGlobal::kBatch)
  {
    if(m_stage)
    {
      // delete previous instance
      if(m_engine)
      {
        m_engine->InvalidateBuffers();
        delete m_engine;
      }

      // combine the excluded paths
      SdfPathVector excludedGeometryPaths;
      excludedGeometryPaths.reserve(m_excludedTaggedGeometry.size() + m_excludedGeometry.size());
      excludedGeometryPaths.assign(m_excludedTaggedGeometry.begin(), m_excludedTaggedGeometry.end());
      excludedGeometryPaths.insert(excludedGeometryPaths.end(), m_excludedGeometry.begin(), m_excludedGeometry.end());

      //
      m_engine = new UsdImagingGLHdEngine(m_path, excludedGeometryPaths);
    }
  }
}


//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::setDependentsDirty(const MPlug& plugBeingDirtied, MPlugArray& plugs)
{
  if(plugBeingDirtied == m_time || plugBeingDirtied == m_timeOffset || plugBeingDirtied == m_timeScalar)
  {
    plugs.append(outTimePlug());
    return MS::kSuccess;
  }
  if(plugBeingDirtied == m_filePath)
  {
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject(), true);
  }
  if (plugBeingDirtied.array() == m_inDrivenTransformsData)
  {
    m_drivenTransformsDirty = true;
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject(), true);
  }
  return MPxSurfaceShape::setDependentsDirty(plugBeingDirtied, plugs);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::preEvaluation(const MDGContext & context, const MEvaluationNode& evaluationNode)
{
  if( !context.isNormal() )
      return MStatus::kFailure;
  MStatus status;
  if (evaluationNode.dirtyPlugExists(m_inDrivenTransformsData, &status) && status)
  {
    m_drivenTransformsDirty = true;
    MHWRender::MRenderer::setGeometryDrawDirty(thisMObject(), true);
  }
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::getRenderAttris(void* pattribs, const MHWRender::MFrameContext& drawRequest, const MDagPath& objPath)
{
  UsdImagingGLEngine::RenderParams& attribs = *(UsdImagingGLEngine::RenderParams*)pattribs;
  uint32_t displayStyle = drawRequest.getDisplayStyle();
  uint32_t displayStatus = MHWRender::MGeometryUtilities::displayStatus(objPath);

  // set wireframe colour
  MColor wireColour = MHWRender::MGeometryUtilities::wireframeColor(objPath);
  attribs.wireframeColor = GfVec4f(wireColour.r, wireColour.g, wireColour.b, wireColour.a);

  // determine the shading mode
  const uint32_t wireframeOnShaded1 = (MHWRender::MFrameContext::kWireFrame | MHWRender::MFrameContext::kGouraudShaded);
  const uint32_t wireframeOnShaded2 = (MHWRender::MFrameContext::kWireFrame | MHWRender::MFrameContext::kFlatShaded);
  if((displayStyle & wireframeOnShaded1) == wireframeOnShaded1 ||
     (displayStyle & wireframeOnShaded2) == wireframeOnShaded2) {
    attribs.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE;
  }
  else
  if(displayStyle & MHWRender::MFrameContext::kWireFrame) {
    attribs.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME;
  }
  else
#if MAYA_API_VERSION >= 201600
  if(displayStyle & MHWRender::MFrameContext::kFlatShaded) {
    attribs.drawMode = UsdImagingGLEngine::DRAW_SHADED_FLAT;
    if ((displayStatus == MHWRender::kActive) ||
        (displayStatus == MHWRender::kLead) ||
        (displayStatus == MHWRender::kHilite)) {
      attribs.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE;
    }
  }
  else
#endif
  if(displayStyle & MHWRender::MFrameContext::kGouraudShaded) {
    attribs.drawMode = UsdImagingGLEngine::DRAW_SHADED_SMOOTH;
    if ((displayStatus == MHWRender::kActive) ||
        (displayStatus == MHWRender::kLead) ||
        (displayStatus == MHWRender::kHilite)) {
      attribs.drawMode = UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE;
    }
  }
  else
  if(displayStyle & MHWRender::MFrameContext::kBoundingBox) {
    attribs.drawMode = UsdImagingGLEngine::DRAW_POINTS;
  }

  // set the time for the scene
  attribs.frame = outTimePlug().asMTime().as(MTime::uiUnit());

#if MAYA_API_VERSION >= 201603
  if(displayStyle & MHWRender::MFrameContext::kBackfaceCulling) {
    attribs.cullStyle = UsdImagingGLEngine::CULL_STYLE_BACK;
  }
  else {
    attribs.cullStyle = UsdImagingGLEngine::CULL_STYLE_NOTHING;
  }
#else
  attribs.cullStyle = UsdImagingGLEngine::CULL_STYLE_NOTHING;
#endif

  const float complexities[] = {1.05f, 1.15f, 1.25f, 1.35f, 1.45f, 1.55f, 1.65f, 1.75f, 1.9f}; 
  attribs.complexity = complexities[complexityPlug().asInt()];
  attribs.showGuides = displayGuidesPlug().asBool();
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::ProxyShape()
  : MPxSurfaceShape(), maya::NodeHelper(),
    m_context(fileio::translators::TranslatorContext::create(this)),
    m_translatorManufacture(context())
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::ProxyShape\n");
  m_beforeSaveSceneId = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, beforeSaveScene, this);
  m_onSelectionChanged = MEventMessage::addEventCallback(MString("SelectionChanged"), onSelectionChanged, this);

  TfWeakPtr<ProxyShape> me(this);

  m_variantChangedNoticeKey = TfNotice::Register(me, &ProxyShape::variantSelectionListener, m_stage);
  m_objectsChangedNoticeKey = TfNotice::Register(me, &ProxyShape::onObjectsChanged, m_stage);
  m_editTargetChanged = TfNotice::Register(me, &ProxyShape::onEditTargetChanged, m_stage);

}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::~ProxyShape()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::~ProxyShape\n");
  MSceneMessage::removeCallback(m_beforeSaveSceneId);
  MNodeMessage::removeCallback(m_attributeChanged);
  MEventMessage::removeCallback(m_onSelectionChanged);
  TfNotice::Revoke(m_variantChangedNoticeKey);
  TfNotice::Revoke(m_objectsChangedNoticeKey);
  TfNotice::Revoke(m_editTargetChanged);
  if(m_engine)
  {
    m_engine->InvalidateBuffers();
    delete m_engine;
  }
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
MStatus ProxyShape::initialise()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::initialise\n");

  const char* errorString = "ProxyShape::initialize";
  try
  {
    setNodeType(kTypeName);
    addFrame("USD Proxy Shape Node");
    m_serializedSessionLayer = addStringAttr("serializedSessionLayer", "ssl", kCached|kReadable|kWritable|kStorable|kHidden);

    m_serializedArCtx = addStringAttr("serializedArCtx", "arcd", kCached|kReadable|kWritable|kStorable|kHidden);
    m_filePath = addFilePathAttr("filePath", "fp", kCached | kReadable | kWritable | kStorable | kAffectsAppearance, kLoad, "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc)");
    m_primPath = addStringAttr("primPath", "pp", kCached | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_excludePrimPaths = addStringAttr("excludePrimPaths", "epp", kCached | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_complexity = addInt32Attr("complexity", "cplx", 0, kCached | kConnectable | kReadable | kWritable | kAffectsAppearance | kKeyable | kStorable);
    setMinMax(m_complexity, 0, 8, 0, 4);
    m_outStageData = addDataAttr("outStageData", "od", StageData::kTypeId, kInternal | kReadable | kWritable | kAffectsAppearance);
    m_displayGuides = addBoolAttr("displayGuides", "dg", false, kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
    m_displayRenderGuides = addBoolAttr("displayRenderGuides", "drg", false, kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
    m_unloaded = addBoolAttr("unloaded", "ul", false, kCached | kKeyable | kWritable | kAffectsAppearance | kStorable);
    m_serializedTrCtx = addStringAttr("serializedTrCtx", "srtc", kReadable|kWritable|kStorable|kHidden);

    addFrame("USD Timing Information");
    m_time = addTimeAttr("time", "tm", MTime(0.0), kCached | kConnectable | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_timeOffset = addTimeAttr("timeOffset", "tmo", MTime(0.0), kCached | kConnectable | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_timeScalar = addDoubleAttr("timeScalar", "tms", 1.0, kCached | kConnectable | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_outTime = addTimeAttr("outTime", "otm", MTime(0.0), kCached | kConnectable | kReadable | kAffectsAppearance);
    m_layers = addMessageAttr("layers", "lys", kWritable | kReadable | kConnectable | kHidden);

    addFrame("USD Driven Transforms");
    m_inDrivenTransformsData = addDataAttr("inDrivenTransformsData", "idrvtd", DrivenTransformsData::kTypeId, kWritable | kArray | kConnectable);

    addFrame("OpenGL Display");
    m_ambient = addColourAttr("ambientColour", "amc", MColor(0.1f, 0.1f, 0.1f), kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
    m_diffuse = addColourAttr("diffuseColour", "dic", MColor(0.7f, 0.7f, 0.7f), kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
    m_specular = addColourAttr("specularColour", "spc", MColor(0.6f, 0.6f, 0.6f), kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
    m_emission = addColourAttr("emissionColour", "emc", MColor(0.0f, 0.0f, 0.0f), kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);
    m_shininess = addFloatAttr("shininess", "shi", 5.0f, kReadable | kWritable | kConnectable | kStorable | kAffectsAppearance);

    m_serializedRefCounts = addStringAttr("serializedRefCounts", "strcs", kReadable | kWritable | kStorable | kHidden);

    AL_MAYA_CHECK_ERROR(attributeAffects(m_time, m_outTime), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_timeOffset, m_outTime), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_timeScalar, m_outTime), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_filePath, m_outStageData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_primPath, m_outStageData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_inDrivenTransformsData, m_outStageData), errorString);
  }
  catch (const MStatus& status)
  {
    return status;
  }

  addBaseTemplate("AEsurfaceShapeTemplate");
  generateAETemplate();

  return MS::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onEditTargetChanged(UsdNotice::StageEditTargetChanged const& notice, UsdStageWeakPtr const& sender)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::onEditTargetChanged\n");
  if (!sender || sender != m_stage)
      return;

  const UsdEditTarget& target = m_stage->GetEditTarget();
  const SdfLayerHandle& layer = target.GetLayer();
  auto layerNode = findLayer(layer);
  if(layerNode)
  {
    layerNode->setHasBeenTheEditTarget(true);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onPrimResync(SdfPath primPath, const SdfPathVector& previousPrims)
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::onPrimResync checking %s\n", primPath.GetText());

  UsdPrim resyncPrim = m_stage->GetPrimAtPath(primPath);
  if(!resyncPrim.IsValid())
  {
    return;
  }

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::onPrimResync begin:\n%s\n", context()->serialise().asChar());

  AL_BEGIN_PROFILE_SECTION(ObjectChanged);
  MFnDagNode fn(thisMObject());
  MDagPath dag_path;
  fn.getPath(dag_path);
  dag_path.pop();

  // find the new set of prims
  std::vector<UsdPrim> newPrimSet = huntForNativeNodesUnderPrim(dag_path, primPath, translatorManufacture());

  proxy::PrimFilter filter(previousPrims, newPrimSet, this);
  m_variantSwitchedPrims.clear();

#if AL_ENABLE_TRACE
  std::cout << "new prims" << std::endl;
  for(auto it : newPrimSet)
  {
    std::cout << it.GetPath().GetText() << std::endl;
  }
  std::cout << "new transforms" << std::endl;
  for(auto it : transformsToCreate)
  {
    std::cout << it.GetPath().GetText() << std::endl;
  }
  std::cout << "updateable prims" << std::endl;
  for(auto it : updatablePrimSet)
  {
    std::cout << it.GetPath().GetText() << std::endl;
  }
  std::cout << "removed prims" << std::endl;
  for(auto it : removedPrimSet)
  {
    std::cout << it.GetText() << std::endl;
  }
#endif

  cmds::ProxyShapePostLoadProcess::MObjectToPrim objsToCreate;
  if(!filter.transformsToCreate().empty())
    cmds::ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims(this, filter.transformsToCreate(), dag_path, objsToCreate);

  if(!filter.newPrimSet().empty())
    cmds::ProxyShapePostLoadProcess::createSchemaPrims(this, filter.newPrimSet());

  if(!filter.updatablePrimSet().empty())
    cmds::ProxyShapePostLoadProcess::updateSchemaPrims(this, filter.updatablePrimSet());

  context()->removeEntries(filter.removedPrimSet());

  cleanupTransformRefs();

  context()->updatePrimTypes();

  // now perform any post-creation fix up
  if(!filter.newPrimSet().empty())
    cmds::ProxyShapePostLoadProcess::connectSchemaPrims(this, filter.newPrimSet());

  //cmds::ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims(this, primsToSwitch, dag_path, objsToCreate);
  if(!filter.updatablePrimSet().empty())
    cmds::ProxyShapePostLoadProcess::connectSchemaPrims(this, filter.updatablePrimSet());


  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::onPrimResync end:\n%s\n", context()->serialise().asChar());

  AL_END_PROFILE_SECTION();

  validateTransforms();
  constructGLImagingEngine();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onObjectsChanged(UsdNotice::ObjectsChanged const& notice, UsdStageWeakPtr const& sender)
{
  if(MFileIO::isOpeningFile())
    return;

  if (!sender || sender != m_stage)
      return;

  TF_DEBUG(ALUSDMAYA_EVENTS).Msg("ProxyShape::onObjectsChanged called m_compositionHasChanged=%i\n", m_compositionHasChanged);
  // These paths are subtree-roots representing entire subtrees that may have
  // changed. In this case, we must dump all cached data below these points
  // and repopulate those trees.
  if(m_compositionHasChanged)
  {
    m_compositionHasChanged = false;

    onPrimResync(m_changedPath, m_variantSwitchedPrims);
    m_variantSwitchedPrims.clear();
    m_changedPath = SdfPath();

    std::stringstream strstr;
    strstr << "Breakdown for Variant Switch:\n";
    maya::Profiler::printReport(strstr);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::validateTransforms()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("validateTransforms\n");
  if(m_stage)
  {
    SdfPathVector pathsToNuke;
    for(auto& it : m_requiredPaths)
    {
      Transform* tm = it.second.m_transform;
      if(!tm)
        continue;

      TransformationMatrix* tmm = tm->transform();
      if(!tmm)
        continue;

      const UsdPrim& prim = tmm->prim();
      if(!prim.IsValid())
      {
        UsdPrim newPrim = m_stage->GetPrimAtPath(it.first);
        if(newPrim)
        {
          std::string transformType;
          newPrim.GetMetadata(Metadata::transformType, &transformType);
          if(newPrim && transformType.empty())
          {
            tm->transform()->setPrim(newPrim);
          }
        }
        else
        {
          pathsToNuke.push_back(it.first);
        }
      }
    }
  }
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("/validateTransforms\n");
}

//----------------------------------------------------------------------------------------------------------------------
std::vector<UsdPrim> ProxyShape::huntForNativeNodesUnderPrim(
    const MDagPath& proxyTransformPath,
    SdfPath startPath,
    fileio::translators::TranslatorManufacture& manufacture)
{

  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::huntForNativeNodesUnderPrim\n");
  std::vector<UsdPrim> prims;
  fileio::SchemaPrimsUtils utils(manufacture);

  fileio::TransformIterator it(m_stage->GetPrimAtPath(startPath), proxyTransformPath);
  for(; !it.done(); it.next())
  {
    UsdPrim prim = it.prim();
    if(!prim.IsValid())
    {
      continue;
    }

    if(utils.isSchemaPrim(prim))
    {
      prims.push_back(prim);
    }
  }
  findExcludedGeometry();
  return prims;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onPrePrimChanged(const SdfPath& path, SdfPathVector& outPathVector)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::onPrePrimChanged\n");
  context()->preRemoveEntry(path, outPathVector);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::variantSelectionListener(SdfNotice::LayersDidChange const& notice, UsdStageWeakPtr const& sender)
// In order to detect changes to the variant selection we listen on the SdfNotice::LayersDidChange global notice which is
// sent to indicate that layer contents have changed.  We are then able to access the change list to check if a variant
// selection change happened.  If so, we trigger a ProxyShapePostLoadProcess() which will regenerate the alTransform
// nodes based on the contents of the new variant selection.
{
  if(MFileIO::isOpeningFile())
    return;

  TF_FOR_ALL(itr, notice.GetChangeListMap())
  {
    TF_FOR_ALL(entryIter, itr->second.GetEntryList())
    {
      const SdfPath &path = entryIter->first;
      const SdfChangeList::Entry &entry = entryIter->second;

      TF_FOR_ALL(it, entry.infoChanged)
      {
        if (it->first == SdfFieldKeys->VariantSelection ||
            it->first == SdfFieldKeys->Active)
        {
          TF_DEBUG(ALUSDMAYA_EVENTS).Msg("ProxyShape::variantSelectionListener oldPath=%s, oldIdentifier=%s, path=%s\n",
                                         entry.oldPath.GetString().c_str(),
                                         entry.oldIdentifier.c_str(),
                                         path.GetText());
          m_compositionHasChanged = true;
          m_changedPath = path;
          onPrePrimChanged(m_changedPath, m_variantSwitchedPrims);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::reloadStage(MPlug& plug)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::reloadStage\n");

  maya::Profiler::clearAll();
  AL_BEGIN_PROFILE_SECTION(ReloadStage);
  MDataBlock dataBlock = forceCache();
  m_stage = UsdStageRefPtr();

  // Get input attr values
  const MString file = inputStringValue(dataBlock, m_filePath);
  const MString serializedSessionLayer = inputStringValue(dataBlock, m_serializedSessionLayer);
  const MString serializedArCtx = inputStringValue(dataBlock, m_serializedArCtx);

  // TODO initialise the context using the serialised attribute

  // let the usd stage cache deal with caching the usd stage data
  std::string fileString = TfStringTrimRight(file.asChar());

  if (not TfStringStartsWith(fileString, "./"))
  {
    fileString = resolvePath(fileString);
  }

  // Fall back on checking if path is just a standard absolute path
  if (fileString.empty())
  {
    fileString.assign(file.asChar(), file.length());
  }

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage called for the usd file: %s\n", fileString.c_str());

  // Check path validity
  // Don't try to create a stage for a non-existent file. Some processes
  // such as mbuild may author a file path here does not yet exist until a
  // later operation (e.g., the mayaConvert target will produce the .mb
  // for the USD standin before the usd target runs the usdModelForeman to
  // assemble all the necessary usd files).
  bool isValidPath = (TfStringStartsWith(fileString, "//") ||
                      TfIsFile(fileString, true /*resolveSymlinks*/));

  if (isValidPath)
  {
    AL_BEGIN_PROFILE_SECTION(OpeningUsdStage);
      AL_BEGIN_PROFILE_SECTION(OpeningSessionLayer);

        SdfLayerRefPtr sessionLayer;
        {
          sessionLayer = SdfLayer::CreateAnonymous();
          if(serializedSessionLayer.length() != 0)
          {
            sessionLayer->ImportFromString(convert(serializedSessionLayer));

            auto layer = getLayer();
            if(layer)
            {
              layer->setLayerAndClearAttribute(sessionLayer);
            }
          }
        }

      AL_END_PROFILE_SECTION();

      AL_BEGIN_PROFILE_SECTION(OpenRootLayer);
        SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(fileString);
      AL_END_PROFILE_SECTION();

      if(rootLayer)
      {
        AL_BEGIN_PROFILE_SECTION(UsdStageOpen);
        UsdStageCacheContext ctx(StageCache::Get());

        bool unloadedFlag = inputBoolValue(dataBlock, m_unloaded);
        UsdStage::InitialLoadSet loadOperation = unloadedFlag ? UsdStage::LoadNone : UsdStage::LoadAll;

        if (sessionLayer)
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage is called with extra session layer.\n");
          m_stage = UsdStage::Open(rootLayer, sessionLayer, loadOperation);
        }
        else
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage is called without any session layer.\n");
          m_stage = UsdStage::Open(rootLayer, loadOperation);
        }

        AL_END_PROFILE_SECTION();
      }
      else
      {
        // file path not valid
        if(file.length())
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage failed to open the usd file: %s.\n", file.asChar());
          MGlobal::displayWarning(MString("Failed to open usd file \"") + file + "\"");
        }
      }
    AL_END_PROFILE_SECTION();
  }
  else
  if(!fileString.empty())
  {
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("The usd file is not valid: %s.\n", file.asChar());
    MGlobal::displayWarning(MString("usd file path not valid \"") + file + "\"");
  }

  // Get the prim
  // If no primPath string specified, then use the pseudo-root.
  const SdfPath rootPath(std::string("/"));
  MString primPathStr = inputStringValue(dataBlock, m_primPath);
  if (primPathStr.length())
  {
    m_path = SdfPath(convert(primPathStr));
    UsdPrim prim = m_stage->GetPrimAtPath(m_path);
    if(!prim)
    {
      m_path = rootPath;
    }
  }
  else
  {
    m_path = rootPath;
  }

  if(m_stage && !MFileIO::isOpeningFile())
  {
    AL_BEGIN_PROFILE_SECTION(PostLoadProcess);
      AL_BEGIN_PROFILE_SECTION(FindExcludedGeometry);
        findExcludedGeometry();
      AL_END_PROFILE_SECTION();

      // execute the post load process to import any custom prims
      cmds::ProxyShapePostLoadProcess::initialise(this);

    AL_END_PROFILE_SECTION();
  }

  AL_END_PROFILE_SECTION();

  if(MGlobal::kInteractive == MGlobal::mayaState())
  {
    std::stringstream strstr;
    strstr << "Breakdown for file: " << file << std::endl;
    maya::Profiler::printReport(strstr);
    MGlobal::displayInfo(convert(strstr.str()));
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructExcludedPrims()
{
  m_excludedGeometry = getExcludePrimPaths();
  constructGLImagingEngine();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug&, void* clientData)
{
  const SdfPath rootPath(std::string("/"));
  ProxyShape* proxy = (ProxyShape*)clientData;
  if(msg & MNodeMessage::kAttributeSet)
  {
    if(plug == m_filePath)
    {
      proxy->reloadStage(plug);
    }
    else
    if(plug == m_primPath)
    {
      if(proxy->m_stage)
      {
        // Get the prim
        // If no primPath string specified, then use the pseudo-root.
        MString primPathStr = plug.asString();
        if (primPathStr.length())
        {
          proxy->m_path = SdfPath(convert(primPathStr));
          UsdPrim prim = proxy->m_stage->GetPrimAtPath(proxy->m_path);
          if(!prim)
          {
            proxy->m_path = rootPath;
          }
        }
        else
        {
          proxy->m_path = rootPath;
        }
        proxy->constructGLImagingEngine();
      }
    }
    else
    if(plug == m_excludePrimPaths)
    {
      if(proxy->m_stage)
      {
        proxy->constructExcludedPrims();
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::postConstructor()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::postConstructor\n");
  setRenderable(true);
  MObject obj = thisMObject();
  m_attributeChanged = MNodeMessage::addAttributeChangedCallback(obj, onAttributeChanged, (void*)this);
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::primHasExcludedParent(UsdPrim prim)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::primHasExcludedParent\n");
  if(prim.IsValid())
  {
    SdfPath primPath = prim.GetPrimPath();
    TF_FOR_ALL(excludedPath, m_excludedTaggedGeometry)
    {
      if (primPath.HasPrefix(*excludedPath))
      {
        return true;
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::findExcludedGeometry()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::findExcludedGeometry\n");
  if(!m_stage)
    return;

  m_excludedTaggedGeometry.clear();
  MDagPath m_parentPath;

  for(fileio::TransformIterator it(m_stage, m_parentPath); !it.done(); it.next())
  {
    const UsdPrim& prim = it.prim();
    if(!prim.IsValid())
      continue;

    bool excludeGeo = false;
    if(prim.GetMetadata(Metadata::excludeFromProxyShape, &excludeGeo))
    {
      if (excludeGeo)
      {
        m_excludedTaggedGeometry.push_back(prim.GetPrimPath());
      }
    }

    // If prim has exclusion tag or is a descendent of a prim with it, create as Maya geo
    if (excludeGeo or primHasExcludedParent(prim))
    {
      VtValue schemaName(fileio::ALExcludedPrimSchema.GetString());
      prim.SetCustomDataByKey(fileio::ALSchemaType, schemaName);
    }
  }
  constructExcludedPrims();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::computeOutStageData(const MPlug& plug, MDataBlock& dataBlock)
{
  // create new stage data
  MObject data;
  StageData* usdStageData = createData<StageData>(StageData::kTypeId, data);
  if(!usdStageData)
  {
    return MS::kFailure;
  }

  // Set the output stage data params
  usdStageData->stage = m_stage;
  usdStageData->primPath = m_path;

  // set the cached output value, and flush
  MStatus status = outputDataValue(dataBlock, m_outStageData, usdStageData);
  if(!status)
  {
    return MS::kFailure;
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::isStageValid() const
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::isStageValid\n");
  MDataBlock dataBlock = const_cast<ProxyShape*>(this)->forceCache();

  StageData* outData = inputDataValue<StageData>(dataBlock, m_outStageData);
  if(outData && outData->stage)
    return true;

  return false;
}

//----------------------------------------------------------------------------------------------------------------------
UsdStageRefPtr ProxyShape::getUsdStage() const
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::getUsdStage\n");

  MPlug plug(thisMObject(), m_outStageData);
  MObject data;
  plug.getValue(data);
  plug.getValue(data);
  MFnPluginData fnData(data);
  StageData* outData = static_cast<StageData*>(fnData.data());
  if(outData)
  {
    return outData->stage;
  }
  return UsdStageRefPtr();
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::computeOutputTime(const MPlug& plug, MDataBlock& dataBlock, MTime& currentTime)
{
  MTime inTime = inputTimeValue(dataBlock, m_time);
  MTime inTimeOffset = inputTimeValue(dataBlock, m_timeOffset);
  double inTimeScalar = inputDoubleValue(dataBlock, m_timeScalar);
  currentTime.setValue((inTime.as(MTime::uiUnit()) - inTimeOffset.as(MTime::uiUnit())) * inTimeScalar);
  return outputTimeValue(dataBlock, m_outTime, currentTime);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::compute(const MPlug& plug, MDataBlock& dataBlock)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::compute %s\n", plug.name().asChar());
  MTime currentTime;
  if(plug == m_outTime)
  {
    return computeOutputTime(plug, dataBlock, currentTime);
  }
  else
  if(plug == m_outStageData)
  {
    MStatus status = computeOutputTime(MPlug(plug.node(), m_outTime), dataBlock, currentTime);
    if (m_drivenTransformsDirty)
    {
      computeDrivenAttributes(plug, dataBlock, currentTime);
    }
    return status == MS::kSuccess ? computeOutStageData(plug, dataBlock) : status;
  }
  return MPxSurfaceShape::compute(plug, dataBlock);
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::isBounded() const
{
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
MBoundingBox ProxyShape::boundingBox() const
{
  MStatus status;

  // Make sure outStage is up to date
  MDataBlock dataBlock = const_cast<ProxyShape*>(this)->forceCache();

  // This would seem to be superfluous? unless it is actually forcing a DG pull?
  MDataHandle outDataHandle = dataBlock.inputValue(m_outStageData, &status);
  CHECK_MSTATUS_AND_RETURN(status, MBoundingBox() );

  // XXX:aluk
  // If we could cheaply determine whether a stage only has static geometry,
  // we could make this value a constant one for that case, avoiding the
  // memory overhead of a cache entry per frame
  UsdTimeCode currTime = UsdTimeCode(inputDoubleValue(dataBlock, m_outTime));

  // RB: There must be a nicer way of doing this that avoids the map?
  // The time codes are likely to be ranged, so an ordered array + binary search would surely work?
  std::map<UsdTimeCode, MBoundingBox>::const_iterator cacheLookup = m_boundingBoxCache.find(currTime);
  if (cacheLookup != m_boundingBoxCache.end())
  {
    return cacheLookup->second;
  }

  GfBBox3d allBox;
  UsdPrim prim = getUsdPrim(dataBlock);
  if (prim)
  {
    UsdGeomImageable imageablePrim(prim);
    bool showGuides = inputBoolValue(dataBlock, m_displayGuides);
    bool showRenderGuides = inputBoolValue(dataBlock, m_displayRenderGuides);
    if (showGuides and showRenderGuides)
    {
      allBox = imageablePrim.ComputeUntransformedBound(
            currTime,
            UsdGeomTokens->default_,
            UsdGeomTokens->proxy,
            UsdGeomTokens->guide,
            UsdGeomTokens->render);
    }
    else
    if (showGuides and not showRenderGuides)
    {
      allBox = imageablePrim.ComputeUntransformedBound(
            currTime,
            UsdGeomTokens->default_,
            UsdGeomTokens->proxy,
            UsdGeomTokens->guide);
    }
    else if (not showGuides and showRenderGuides)
    {
      allBox = imageablePrim.ComputeUntransformedBound(
            currTime,
            UsdGeomTokens->default_,
            UsdGeomTokens->proxy,
            UsdGeomTokens->render);
    }
    else
    {
      allBox = imageablePrim.ComputeUntransformedBound(
            currTime,
            UsdGeomTokens->default_,
            UsdGeomTokens->proxy);
    }
  }
  else
  {
    return MBoundingBox();
  }

  // insert new cache entry
  MBoundingBox& retval = m_boundingBoxCache[currTime];

  // Convert to GfRange3d to MBoundingBox
  GfRange3d boxRange = allBox.ComputeAlignedBox();
  if (!boxRange.IsEmpty())
  {
    retval = MBoundingBox(MPoint(boxRange.GetMin()[0],
                                 boxRange.GetMin()[1],
                                 boxRange.GetMin()[2]),
                          MPoint(boxRange.GetMax()[0],
                                 boxRange.GetMax()[1],
                                 boxRange.GetMax()[2]));
  }
  else
  {
    retval = MBoundingBox(MPoint(-100000.0f, -100000.0f, -100000.0f), MPoint(100000.0f, 100000.0f, 100000.0f));
  }

  return retval;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::unloadMayaReferences()
{
  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::unloadMayaReferences called\n");
  MObjectArray references;
  for(auto it = m_requiredPaths.begin(), e = m_requiredPaths.end(); it != e; ++it)
  {
    MStatus status;
    MFnDependencyNode fn(it->second.node(), &status);
    if(status)
    {
      MPlug plug = fn.findPlug("message", &status);
      if(status)
      {
        MPlugArray plugs;
        plug.connectedTo(plugs, false, true);
        for(uint32_t i = 0; i < plugs.length(); ++i)
        {
          MObject temp = plugs[i].node();
          if(temp.hasFn(MFn::kReference))
          {
            MString command = MString("referenceQuery -filename ") + MFnDependencyNode(temp).name();
            MString referenceFilename;
            MStatus returnStatus = MGlobal::executeCommand(command, referenceFilename);
            if (returnStatus != MStatus::kFailure)
            {
              TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::unloadMayaReferences removing %s\n", referenceFilename.asChar());
              MFileIO::removeReference(referenceFilename);
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
MStatus ProxyShape::computeDrivenAttributes(const MPlug& plug, MDataBlock& dataBlock, const MTime& currentTime)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::computeDrivenAttributes\n");
  m_drivenTransformsDirty = false;
  MArrayDataHandle drvTransArray = dataBlock.inputArrayValue(m_inDrivenTransformsData);
  uint32_t elemCnt = drvTransArray.elementCount();
  for (uint32_t elemIdx = 0; elemIdx < elemCnt; ++elemIdx)
  {
    drvTransArray.jumpToArrayElement(elemIdx);
    MDataHandle dtHandle = drvTransArray.inputValue();
    DrivenTransformsData* dtData = static_cast<DrivenTransformsData*>(dtHandle.asPluginData());
    if (!dtData)
      continue;
    proxy::DrivenTransforms& drivenTransforms = dtData->m_drivenTransforms;
    if (elemIdx >= m_drivenPaths.size())
    {
      m_drivenPaths.resize(elemIdx + 1);
      m_drivenPrims.resize(elemIdx + 1);
    }
    std::vector<SdfPath>& drivenPaths = m_drivenPaths[elemIdx];
    std::vector<UsdPrim>& drivenPrims = m_drivenPrims[elemIdx];

    if (!drivenTransforms.drivenPrimPaths().empty())
    {
      drivenTransforms.updateDrivenPrimPaths(elemIdx, drivenPaths, drivenPrims, m_stage);
    }
    if (!drivenTransforms.dirtyMatrices().empty())
    {
      drivenTransforms.updateDrivenTransforms(drivenPrims, currentTime);
    }
    if (!drivenTransforms.dirtyVisibilities().empty())
    {
      drivenTransforms.updateDrivenVisibility(drivenPrims, currentTime);
    }
  }
  return dataBlock.setClean(plug);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialiseTransformRefs()
{
  std::ostringstream oss;
  for(auto iter : m_requiredPaths)
  {
    MFnDagNode fn(iter.second.node());
    MDagPath path;
    fn.getPath(path);
    oss << path.fullPathName() << " "
        << iter.first.GetText() << " "
        << uint32_t(iter.second.required()) << " "
        << uint32_t(iter.second.selected()) << " "
        << uint32_t(iter.second.refCount()) << ";";
  }
  serializedRefCountsPlug().setString(oss.str().c_str());
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::deserialiseTransformRefs()
{
  MString str = serializedRefCountsPlug().asString();
  MStringArray strs;
  str.split(';', strs);

  for(uint32_t i = 0, n = strs.length(); i < n; ++i)
  {
    if(strs[i].length())
    {
      MStringArray tstrs;
      strs[i].split(' ', tstrs);
      MString nodeName = tstrs[0];

      MSelectionList sl;
      if(sl.add(nodeName))
      {
        MObject node;
        if(sl.getDependNode(0, node))
        {
          MFnDependencyNode fn(node);
          if(fn.typeId() == AL_USDMAYA_TRANSFORM)
          {
            Transform* ptr = (Transform*)fn.userNode();
            const uint32_t required = tstrs[2].asUnsigned();
            const uint32_t selected = tstrs[3].asUnsigned();
            const uint32_t refCounts = tstrs[4].asUnsigned();
            SdfPath path(tstrs[1].asChar());
            m_requiredPaths.emplace(path, TransformReference(node, ptr, required, selected, refCounts));
          }
          else
          {
            const uint32_t required = tstrs[2].asUnsigned();
            const uint32_t selected = tstrs[3].asUnsigned();
            const uint32_t refCounts = tstrs[4].asUnsigned();
            SdfPath path(tstrs[1].asChar());
            m_requiredPaths.emplace(path, TransformReference(node, 0, required, selected, refCounts));
          }
        }
      }
    }
  }

  serializedRefCountsPlug().setString("");
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::TransformReference::TransformReference(MObject mayaNode, Transform* node, uint32_t r, uint32_t s, uint32_t rc)
  : m_node(mayaNode), m_transform(node)
{
  m_required = r;
  m_selected = s;
  m_refCount = rc;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::cleanupTransformRefs()
{
  for(auto it = m_requiredPaths.begin(); it != m_requiredPaths.end(); )
  {
    if(!it->second.selected() && !it->second.required() && !it->second.refCount())
    {
      m_requiredPaths.erase(it++);
    }
    else
    {
      ++it;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
