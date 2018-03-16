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

#if (__cplusplus >= 201703L)
# include <filesystem>
#else
# include <boost/filesystem.hpp>
#endif

namespace AL {
namespace filesystem {
#if (__cplusplus >= 201703L)
typedef std::filesystem::path path;
#else
typedef boost::filesystem::path path;
#endif
}
}

#include "AL/usdmaya/CodeTimings.h"
#include "AL/usdmaya/utils/Utils.h"

#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/Global.h"
#include "AL/usdmaya/Metadata.h"
#include "AL/usdmaya/StageCache.h"
#include "AL/usdmaya/StageData.h"
#include "AL/usdmaya/TypeIDs.h"

#include "AL/usdmaya/cmds/ProxyShapePostLoadProcess.h"
#include "AL/usdmaya/fileio/SchemaPrims.h"
#include "AL/usdmaya/fileio/TransformIterator.h"
#include "AL/usdmaya/nodes/LayerManager.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/nodes/proxy/PrimFilter.h"
#include "AL/usdmaya/Version.h"
#include "AL/usd/utils/ForwardDeclares.h"

#include "maya/MFileIO.h"
#include "maya/MFnPluginData.h"
#include "maya/MFnReference.h"
#include "maya/MHWGeometryUtilities.h"
#include "maya/MItDependencyNodes.h"
#include "maya/MPlugArray.h"
#include "maya/MNodeClass.h"
#include "maya/MFileIO.h"
#include "maya/MCommandResult.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stageCacheContext.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/meshAdapter.h"

#include <algorithm>
#include <iterator>

namespace AL {
namespace usdmaya {
namespace nodes {
typedef void (*proxy_function_prototype)(void* userData, AL::usdmaya::nodes::ProxyShape* proxyInstance);

const char* ProxyShape::s_selectionMaskName = "al_ProxyShape";

MDagPath ProxyShape::parentTransform()
{
  MFnDagNode fn(thisMObject());
  MDagPath proxyTransformPath;
  fn.getPath(proxyTransformPath);
  proxyTransformPath.pop();
  return proxyTransformPath;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialiseTranslatorContext()
{
  triggerEvent("PreSerialiseContext");

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
static std::string resolvePath(const std::string& filePath)
{
  ArResolver& resolver = ArGetResolver();

  return resolver.Resolve(filePath);
}

static std::string getDir(const std::string &fullFilePath)
{
  return AL::filesystem::path(fullFilePath).parent_path().string();
}

static std::string getMayaReferencedFileDir(const MObject &proxyShapeNode)
{
  // Can not use MFnDependencyNode(proxyShapeNode).isFromReferencedFile() to test if it is reference node or not,
  // which always return false even the proxyShape node is referenced...

  MStatus stat;
  MFnReference refFn;
  MItDependencyNodes dgIter(MFn::kReference, &stat);
  for (; !dgIter.isDone(); dgIter.next())
  {
    MObject cRefNode = dgIter.thisNode();
    refFn.setObject(cRefNode);
    if(refFn.containsNodeExactly(proxyShapeNode, &stat))
    {
      // According to Maya API document, the second argument is 'includePath' and set it to true to include the file path.
      // However, I have to set it to false to return the full file path otherwise I get a file name only...
      MString refFilePath = refFn.fileName(true, false, false, &stat);
      if(!refFilePath.length())
        return std::string();

      std::string referencedFilePath = refFilePath.asChar();
      TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("getMayaReferencedFileDir: The reference file that contains the proxyShape node is : %s\n", referencedFilePath.c_str());

      return getDir(referencedFilePath);
    }
  }

  return std::string();
}

static std::string getMayaSceneFileDir()
{
  std::string currentFile = AL::maya::utils::convert(MFileIO::currentFile());
  size_t filePathSize = currentFile.size();
  if(filePathSize < 4)
    return std::string();

  // If scene is untitled, the maya file will be MayaWorkspaceDir/untitled :
  constexpr char ma_ext[] = ".ma";
  constexpr char mb_ext[] = ".mb";
  auto ext_start = currentFile.end() - 3;
  if(std::equal(ma_ext, ma_ext + 3, ext_start) ||
     std::equal(mb_ext, mb_ext + 3, ext_start))
    return getDir(currentFile);

  return std::string();
}

static std::string resolveRelativePathWithinMayaContext(const MObject &proxyShape, const std::string& relativeFilePath)
{
  if (relativeFilePath.length() < 3)
    return relativeFilePath;

  std::string currentFileDir = getMayaReferencedFileDir(proxyShape);
  if(currentFileDir.empty())
    currentFileDir = getMayaSceneFileDir();

  if(currentFileDir.empty())
    return relativeFilePath;

  AL::filesystem::path path = boost::filesystem::canonical(relativeFilePath, currentFileDir);
  return path.string();
}

//----------------------------------------------------------------------------------------------------------------------
AL_MAYA_DEFINE_NODE(ProxyShape, AL_USDMAYA_PROXYSHAPE, AL_usdmaya);

MObject ProxyShape::m_filePath = MObject::kNullObj;
MObject ProxyShape::m_primPath = MObject::kNullObj;
MObject ProxyShape::m_excludePrimPaths = MObject::kNullObj;
MObject ProxyShape::m_populationMaskIncludePaths = MObject::kNullObj;
MObject ProxyShape::m_excludedTranslatedGeometry = MObject::kNullObj;
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
MObject ProxyShape::m_sessionLayerName = MObject::kNullObj;
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
MObject ProxyShape::m_version = MObject::kNullObj;
MObject ProxyShape::m_transformTranslate = MObject::kNullObj;
MObject ProxyShape::m_transformRotate = MObject::kNullObj;
MObject ProxyShape::m_transformScale = MObject::kNullObj;
MObject ProxyShape::m_stageDataDirty = MObject::kNullObj;

//----------------------------------------------------------------------------------------------------------------------
std::vector<MObjectHandle> ProxyShape::m_unloadedProxyShapes;

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

  MString paths = excludePrimPathsPlug().asString();
  return getPrimPathsFromCommaJoinedString(paths);
}

//----------------------------------------------------------------------------------------------------------------------
UsdStagePopulationMask ProxyShape::constructStagePopulationMask(const MString &paths) const
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::constructStagePopulationMask(%s)\n", paths.asChar());
  UsdStagePopulationMask mask;
  SdfPathVector list = getPrimPathsFromCommaJoinedString(paths);
  if(list.empty())
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape: No mask specified, will mask none.\n");
    return UsdStagePopulationMask::All();
  }

  for(const SdfPath &path : list)
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape: Add include to mask:(%s)\n", path.GetString().c_str());
    mask.Add(path);
  }
  return mask;
}

void ProxyShape::translatePrimPathsIntoMaya(
    const SdfPathVector& importPaths,
    const SdfPathVector& teardownPaths,
    const fileio::translators::TranslatorParameters& param)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape:translatePrimPathsIntoMaya ImportSize='%d' TearDownSize='%d' \n",
                                     importPaths.size(),
                                     teardownPaths.size());

  //Resolve SdfPathSet to UsdPrimVector
  UsdPrimVector importPrims;
  for(const SdfPath& path : importPaths)
  {
    UsdPrim prim = m_stage->GetPrimAtPath(path);
    if(prim.IsValid())
    {
      importPrims.push_back(prim);
    }
    else
    {
      TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape:translatePrimPathsIntoMaya Path for import '%s' resolves to an invalid prim\n", path.GetString().c_str());
    }
  }

  translatePrimsIntoMaya(importPrims, teardownPaths, param);
}

void ProxyShape::translatePrimsIntoMaya(
    const UsdPrimVector& importPrims,
    const SdfPathVector& teardownPrims,
    const fileio::translators::TranslatorParameters& param)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape:translatePrimsIntoMaya ImportSize='%d' TearDownSize='%d' \n", importPrims.size(), teardownPrims.size());

  proxy::PrimFilter filter(teardownPrims, importPrims, this);
  if(TfDebug::IsEnabled(ALUSDMAYA_TRANSLATORS))
  {
    std::cout << "new prims" << std::endl;
    for(auto it : filter.newPrimSet())
    {
      std::cout << it.GetPath().GetText() << std::endl;
    }
    std::cout << "new transforms" << std::endl;
    for(auto it : filter.transformsToCreate())
    {
      std::cout << it.GetPath().GetText() << std::endl;
    }
    std::cout << "updateable prims" << std::endl;
    for(auto it : filter.updatablePrimSet())
    {
      std::cout << it.GetPath().GetText() << std::endl;
    }
    std::cout << "removed prims" << std::endl;
    for(auto it : filter.removedPrimSet())
    {
      std::cout << it.GetText() << std::endl;
    }
  }

  cmds::ProxyShapePostLoadProcess::MObjectToPrim objsToCreate;
  if(!filter.transformsToCreate().empty())
  {
    cmds::ProxyShapePostLoadProcess::createTranformChainsForSchemaPrims(
        this,
        filter.transformsToCreate(),
        parentTransform(),
        objsToCreate);
  }

  context()->removeEntries(filter.removedPrimSet());

  if(!filter.newPrimSet().empty())
  {
    cmds::ProxyShapePostLoadProcess::createSchemaPrims(this, filter.newPrimSet(), param);
  }

  if(!filter.updatablePrimSet().empty())
  {
    cmds::ProxyShapePostLoadProcess::updateSchemaPrims(this, filter.updatablePrimSet());
  }

  cleanupTransformRefs();

  context()->updatePrimTypes();

  // now perform any post-creation fix up
  if(!filter.newPrimSet().empty())
  {
    cmds::ProxyShapePostLoadProcess::connectSchemaPrims(this, filter.newPrimSet());
  }

  if(!filter.updatablePrimSet().empty())
  {
    cmds::ProxyShapePostLoadProcess::connectSchemaPrims(this, filter.updatablePrimSet());
  }

  if(context()->isExcludedGeometryDirty())
  {
    TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape:translatePrimsIntoMaya excluded geometry has been modified, reconstructing imaging engine \n");
    constructGLImagingEngine();
  }
}
//----------------------------------------------------------------------------------------------------------------------
SdfPathVector ProxyShape::getPrimPathsFromCommaJoinedString(const MString &paths) const
{
  SdfPathVector result;
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
      // function prototype of callback we wish to register
      typedef void (*proxy_function_prototype)(void*, AL::usdmaya::nodes::ProxyShape*);

      // delete previous instance
      if(m_engine)
      {
        triggerEvent("DestroyGLEngine");
        m_engine->InvalidateBuffers();
        delete m_engine;
      }

      const SdfPathSet& translatedGeo = m_context->excludedGeometry();
      // combine the excluded paths
      SdfPathVector excludedGeometryPaths;
      excludedGeometryPaths.reserve(m_excludedTaggedGeometry.size() + m_excludedGeometry.size() + translatedGeo.size());
      excludedGeometryPaths.assign(m_excludedTaggedGeometry.begin(), m_excludedTaggedGeometry.end());
      excludedGeometryPaths.insert(excludedGeometryPaths.end(), m_excludedGeometry.begin(), m_excludedGeometry.end());
      excludedGeometryPaths.insert(excludedGeometryPaths.end(),
                                   translatedGeo.begin(),
                                   translatedGeo.end());

      m_engine = new UsdImagingGLHdEngine(m_path, excludedGeometryPaths);

      triggerEvent("ConstructGLEngine");
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
  : MPxSurfaceShape(), AL::maya::utils::NodeHelper(), AL::event::NodeEvents(&AL::event::EventScheduler::getScheduler()),
    m_context(fileio::translators::TranslatorContext::create(this)),
    m_translatorManufacture(context())
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::ProxyShape\n");
  m_onSelectionChanged = MEventMessage::addEventCallback(MString("SelectionChanged"), onSelectionChanged, this);

  TfWeakPtr<ProxyShape> me(this);

  m_variantChangedNoticeKey = TfNotice::Register(me, &ProxyShape::variantSelectionListener);
  m_objectsChangedNoticeKey = TfNotice::Register(me, &ProxyShape::onObjectsChanged, m_stage);
  m_editTargetChanged = TfNotice::Register(me, &ProxyShape::onEditTargetChanged, m_stage);

  registerEvents();

  m_findExcludedPrims.preIteration = [this]() {
    m_excludedTaggedGeometry.clear();
  };
  m_findExcludedPrims.iteration = [this]( const fileio::TransformIterator& transformIterator,
                                          const UsdPrim& prim) {

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
  };
  m_findExcludedPrims.postIteration = [this]() {
    constructExcludedPrims();
  };

  m_findUnselectablePrims.preIteration = [this]() {

  };
  m_findUnselectablePrims.iteration = [this]
                                    (const fileio::TransformIterator& transformIterator, const UsdPrim& prim) {

    TfToken selectabilityPropertyToken;
    if(prim.GetMetadata<TfToken>(Metadata::selectability, &selectabilityPropertyToken))
    {

      //Check if this prim is unselectable
      if(selectabilityPropertyToken == Metadata::unselectable)
      {
        m_findUnselectablePrims.newUnselectables.push_back(prim.GetPath());
      }
      else if(m_selectabilityDB.isPathUnselectable(prim.GetPath()) && selectabilityPropertyToken != Metadata::unselectable)
      {
        m_findUnselectablePrims.removeUnselectables.push_back(prim.GetPath());
      }
    }
  };
  m_findUnselectablePrims.postIteration = [this]() {
    if(m_findUnselectablePrims.removeUnselectables.size() > 0)
    {
      m_selectabilityDB.removePathsAsUnselectable(m_findUnselectablePrims.removeUnselectables);
    }

    if(m_findUnselectablePrims.newUnselectables.size() > 0)
    {
      m_selectabilityDB.addPathsAsUnselectable(m_findUnselectablePrims.newUnselectables);
    }

    m_findUnselectablePrims.newUnselectables.clear();
    m_findUnselectablePrims.removeUnselectables.clear();
  };

  m_findLockedPrims.preIteration = [this]() {
    this->m_lockTransformPrims.clear();
    this->m_lockInheritedPrims.clear();
  };
  m_findLockedPrims.iteration = [this] ( const fileio::TransformIterator& transformIterator,
                                         const UsdPrim& prim)
  {
    TfToken lockPropertyToken;
    if (prim.GetMetadata<TfToken>(Metadata::locked, & lockPropertyToken))
    {
      if (lockPropertyToken == Metadata::lockTransform)
      {
        this->m_lockTransformPrims.insert(prim.GetPath());
      }
      else if (lockPropertyToken == Metadata::lockInherited)
      {
        this->m_lockInheritedPrims.insert(prim.GetPath());
      }
    }
    else
    {
      this->m_lockInheritedPrims.insert(prim.GetPath());
    }

  };
  m_findLockedPrims.postIteration = [this]() {
    constructLockPrims();
  };

  m_hierarchyIterationLogics[0] = &m_findExcludedPrims;
  m_hierarchyIterationLogics[1] = &m_findUnselectablePrims;
  m_hierarchyIterationLogics[2] = &m_findLockedPrims;
}

//----------------------------------------------------------------------------------------------------------------------
ProxyShape::~ProxyShape()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::~ProxyShape\n");
  MNodeMessage::removeCallback(m_attributeChanged);
  MEventMessage::removeCallback(m_onSelectionChanged);
  removeAttributeChangedCallback();
  TfNotice::Revoke(m_variantChangedNoticeKey);
  TfNotice::Revoke(m_objectsChangedNoticeKey);
  TfNotice::Revoke(m_editTargetChanged);
  if(m_engine)
  {
    triggerEvent("DestroyGLEngine");
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
    m_sessionLayerName = addStringAttr("sessionLayerName", "sln", kCached|kReadable|kWritable|kStorable|kHidden);

    m_serializedArCtx = addStringAttr("serializedArCtx", "arcd", kCached|kReadable|kWritable|kStorable|kHidden);
    m_filePath = addFilePathAttr("filePath", "fp", kCached | kReadable | kWritable | kStorable | kAffectsAppearance, kLoad, "USD Files (*.usd*) (*.usd*);;Alembic Files (*.abc)");

    m_primPath = addStringAttr("primPath", "pp", kCached | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_excludePrimPaths = addStringAttr("excludePrimPaths", "epp", kCached | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_populationMaskIncludePaths = addStringAttr("populationMaskIncludePaths", "pmi", kCached | kReadable | kWritable | kStorable | kAffectsAppearance);
    m_excludedTranslatedGeometry = addStringAttr("excludedTranslatedGeometry", "etg", kCached | kReadable | kWritable | kStorable | kAffectsAppearance);

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

    m_version = addStringAttr(
        "version", "vrs", getVersion().c_str(),
        kReadable | kStorable | kHidden
        );

    MNodeClass nc("transform");
    m_transformTranslate = nc.attribute("t");
    m_transformRotate = nc.attribute("r");
    m_transformScale = nc.attribute("s");

    m_stageDataDirty = addBoolAttr("stageDataDirty", "sdd", false, kWritable | kAffectsAppearance | kInternal);

    AL_MAYA_CHECK_ERROR(attributeAffects(m_time, m_outTime), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_timeOffset, m_outTime), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_timeScalar, m_outTime), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_filePath, m_outStageData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_primPath, m_outStageData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_inDrivenTransformsData, m_outStageData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_populationMaskIncludePaths, m_outStageData), errorString);
    AL_MAYA_CHECK_ERROR(attributeAffects(m_stageDataDirty, m_outStageData), errorString);
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

  trackEditTargetLayer();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::trackEditTargetLayer(LayerManager* layerManager)
{
  TF_DEBUG(ALUSDMAYA_LAYERS).Msg("ProxyShape::trackEditTargetLayer");
  auto stage = getUsdStage();
  if(!stage)
  {
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg(" - no stage\n");
    return;
  }

  auto prevTargetLayer = m_prevTargetLayer;
  m_prevTargetLayer = stage->GetEditTarget().GetLayer();

  if(!prevTargetLayer)
  {
    TF_DEBUG(ALUSDMAYA_LAYERS).Msg(" - no prev target layer\n");
    return;
  }

  TF_DEBUG(ALUSDMAYA_LAYERS).Msg(" - prev target layer: %s\n",
      prevTargetLayer->GetIdentifier().c_str());
  if(prevTargetLayer->IsDirty())
  {
    if(!layerManager)
    {
      layerManager = LayerManager::findOrCreateManager();
      // findOrCreateManager SHOULD always return a result, but we check anyway,
      // to avoid any potential crash...
      if(!layerManager)
      {
        std::cerr << "Error creating / finding a layerManager node!" << std::endl;
        return;
      }
    }
    layerManager->addLayer(prevTargetLayer);
  }
  triggerEvent("EditTargetChanged");
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onPrimResync(SdfPath primPath, SdfPathVector& previousPrims)
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
  MDagPath proxyTransformPath;
  fn.getPath(proxyTransformPath);
  proxyTransformPath.pop();

  // find the new set of prims
  UsdPrimVector newPrimSet = huntForNativeNodesUnderPrim(proxyTransformPath, primPath, translatorManufacture());

  // Remove prims that have disappeared and translate in new prims
  translatePrimsIntoMaya(newPrimSet, previousPrims);

  previousPrims.clear();

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::onPrimResync end:\n%s\n", context()->serialise().asChar());

  AL_END_PROFILE_SECTION();

  validateTransforms();
  constructGLImagingEngine();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::resync(const SdfPath& primPath)
{
  // FIMXE: This method was needed to call update() on all translators in the maya scene. Since then some new
  // locking and selectability functionality has been added to onObjectsChanged(). I would want to call the logic in
  // that method to handle this resyncing but it would need to be refactored.

  SdfPathVector existingSchemaPrims;

  // populates list of prims from prim mapping that will change under the path to resync.
  onPrePrimChanged(primPath, existingSchemaPrims);

  onPrimResync(primPath, existingSchemaPrims);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialize(UsdStageRefPtr stage, LayerManager* layerManager)
{
  if(stage)
  {
    if (layerManager)
    {
      // Make sure the sessionLayer is always serialized (even if it's never an edit target)
      auto sessionLayer = stage->GetSessionLayer();
      layerManager->addLayer(sessionLayer);
      // ...and store the name for the (anonymous) session layer so we can find it!
      sessionLayerNamePlug().setValue(AL::maya::utils::convert(sessionLayer->GetIdentifier()));

      // Then add in the current edit target
      trackEditTargetLayer(layerManager);
    }
    else
    {
      MGlobal::displayError("ProxyShape::serialize was passed a nullptr for the layerManager");
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
    for(; !iter.isDone(); iter.next())
    {
      MObject mobj = iter.item();
      fn.setObject(mobj);
      if(fn.typeId() != ProxyShape::kTypeId) continue;

      if (layerManager == nullptr)
      {
        layerManager = LayerManager::findOrCreateManager();
      }

      if(!layerManager)
      {
        MGlobal::displayError(MString("Error creating layerManager"));
        continue;
      }

      auto proxyShape = static_cast<ProxyShape *>(fn.userNode());
      if(proxyShape == nullptr)
      {
        MGlobal::displayError(MString("ProxyShape had no mpx data: ") + fn.name());
        continue;
      }

      UsdStageRefPtr stage = proxyShape->getUsdStage();

      if(!stage)
      {
        MGlobal::displayError(MString("Could not get stage for proxyShape: ") + fn.name());
        continue;
      }

      proxyShape->serialize(stage, layerManager);
    }

    // Bail if no proxyShapes were found...
    if(!layerManager) return;

    // Now that all layers are added, serialize to attributes
    AL_MAYA_CHECK_ERROR_RETURN(layerManager->populateSerialisationAttributes(), errorString);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onObjectsChanged(UsdNotice::ObjectsChanged const& notice, UsdStageWeakPtr const& sender)
{
  if(MFileIO::isReadingFile())
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
    AL::usdmaya::Profiler::printReport(strstr);
  }

  SdfPathVector newUnselectables;
  SdfPathVector removeUnselectables;
  auto recordSelectablePrims = [&newUnselectables, &removeUnselectables, this](const UsdPrim& prim){
    if(!prim.IsValid())
    {
      return;
    }

    TfToken unselectablePropertyValue;
    if(prim.GetMetadata(Metadata::selectability, &unselectablePropertyValue))
    {
      //Check if this prim is unselectable
      if(unselectablePropertyValue == Metadata::unselectable)
      {
        newUnselectables.push_back(prim.GetPath());
      }
      else if(m_selectabilityDB.isPathUnselectable(prim.GetPath()) && unselectablePropertyValue != Metadata::unselectable)
      {
        removeUnselectables.push_back(prim.GetPath());
      }
    }
  };

  SdfPathSet lockTransformPrims;
  SdfPathSet lockInheritedPrims;
  SdfPathSet unlockedPrims;
  auto recordPrimsLockStatus = [&lockTransformPrims, &lockInheritedPrims, &unlockedPrims](const UsdPrim& prim) {
    if (!prim.IsValid())
    {
      return;
    }
    TfToken lockPropertyValue;
    if (prim.GetMetadata(Metadata::locked, &lockPropertyValue))
    {
      if (lockPropertyValue == Metadata::lockTransform)
      {
        lockTransformPrims.insert(prim.GetPath());
      }
      else if (lockPropertyValue == Metadata::lockInherited)
      {
        lockInheritedPrims.insert(prim.GetPath());
      }
      else if (lockPropertyValue == Metadata::lockUnlocked)
      {
        unlockedPrims.insert(prim.GetPath());
      }
    }
    else
    {
      lockInheritedPrims.insert(prim.GetPath());
    }
  };

  const SdfPathVector& resyncedPaths = notice.GetResyncedPaths();
  for(const SdfPath& path : resyncedPaths)
  {
    UsdPrim newPrim = m_stage->GetPrimAtPath(path);
    recordSelectablePrims(newPrim);
    recordPrimsLockStatus(newPrim);
  }

  const SdfPathVector& changedInfoOnlyPaths = notice.GetChangedInfoOnlyPaths();
  for(const SdfPath& path : changedInfoOnlyPaths)
  {
    UsdPrim changedPrim;
    if(path.IsPropertyPath())
    {
      changedPrim = m_stage->GetPrimAtPath(path.GetParentPath());
    }
    else
    {
      changedPrim = m_stage->GetPrimAtPath(path);
    }
    recordSelectablePrims(changedPrim);
    recordPrimsLockStatus(changedPrim);
  }

  if(!removeUnselectables.empty())
  {
    m_selectabilityDB.removePathsAsUnselectable(removeUnselectables);
  }

  if(!newUnselectables.empty())
  {
    m_selectabilityDB.addPathsAsUnselectable(newUnselectables);
  }

  bool lockChanged = updateLockPrims(lockTransformPrims, lockInheritedPrims, unlockedPrims);
  if (lockChanged)
  {
    constructLockPrims();
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

    fileio::translators::TranslatorRefPtr trans = utils.isSchemaPrim(prim);
    if(trans && trans->importableByDefault())
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
void ProxyShape::variantSelectionListener(SdfNotice::LayersDidChange const& notice)
// In order to detect changes to the variant selection we listen on the SdfNotice::LayersDidChange global notice which is
// sent to indicate that layer contents have changed.  We are then able to access the change list to check if a variant
// selection change happened.  If so, we trigger a ProxyShapePostLoadProcess() which will regenerate the alTransform
// nodes based on the contents of the new variant selection.
{
  if(MFileIO::isReadingFile())
  {
    return;
  }

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
          triggerEvent("PreVariantChangedCB");

          TF_DEBUG(ALUSDMAYA_EVENTS).Msg("ProxyShape::variantSelectionListener oldPath=%s, oldIdentifier=%s, path=%s, layer=%s\n",
                                         entry.oldPath.GetString().c_str(),
                                         entry.oldIdentifier.c_str(),
                                         path.GetText(),
                                         itr->first->GetIdentifier().c_str());
          if(!m_compositionHasChanged)
          {
            TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::Not yet in a composition change state. Recording path. \n");
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

  AL_BEGIN_PROFILE_SECTION(LoadStage);
  MDataBlock dataBlock = forceCache();
  // in case there was already a stage in m_stage, check to see if it's edit target has been altered
  trackEditTargetLayer();
  m_stage = UsdStageRefPtr();

  // Get input attr values
  const MString file = inputStringValue(dataBlock, m_filePath);
  const MString sessionLayerName = inputStringValue(dataBlock, m_sessionLayerName);
  const MString serializedArCtx = inputStringValue(dataBlock, m_serializedArCtx);

  const MString populationMaskIncludePaths = inputStringValue(dataBlock, m_populationMaskIncludePaths);
  UsdStagePopulationMask mask = constructStagePopulationMask(populationMaskIncludePaths);

  // TODO initialise the context using the serialised attribute

  // let the usd stage cache deal with caching the usd stage data
  std::string fileString = TfStringTrimRight(file.asChar());

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage original USD file path is %s\n", fileString.c_str());

  AL::filesystem::path filestringPath (fileString);
  if(filestringPath.is_absolute())
  {
    fileString = resolvePath(fileString);
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage resolved the USD file path to %s\n", fileString.c_str());
  }
  else
  {
    fileString = resolveRelativePathWithinMayaContext(thisMObject(), fileString);
    TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::reloadStage resolved the relative USD file path to %s\n", fileString.c_str());
  }

  // Fall back on checking if path is just a standard absolute path
  if (fileString.empty())
  {
    fileString.assign(file.asChar(), file.length());
  }

  TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::loadStage called for the usd file: %s\n", fileString.c_str());

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
    MStatus status;
    SdfLayerRefPtr sessionLayer;

    AL_BEGIN_PROFILE_SECTION(OpeningUsdStage);
      AL_BEGIN_PROFILE_SECTION(OpeningSessionLayer);
        {
          // Grab the session layer from the layer manager
          if(sessionLayerName.length() > 0)
          {
            auto layerManager = LayerManager::findManager();
            if(layerManager)
            {
              sessionLayer = layerManager->findLayer(AL::maya::utils::convert(sessionLayerName));
              if(!sessionLayer)
              {
                MGlobal::displayError(MString("ProxyShape \"") + name() + "\" had a serialized session layer"
                    " named \"" + sessionLayerName + "\", but no matching layer could be found in the layerManager");
              }
            }
            else
            {
              MGlobal::displayError(MString("ProxyShape \"") + name() + "\" had a serialized session layer,"
                  " but no layerManager node was found");
            }
          }

          // If we still have no sessionLayer, but there's data in serializedSessionLayer, then
          // assume we're reading an "old" file, and read it for backwards compatibility.
          if(!sessionLayer)
          {
            const MString serializedSessionLayer = inputStringValue(dataBlock, m_serializedSessionLayer);
            if(serializedSessionLayer.length() != 0)
            {
              sessionLayer = SdfLayer::CreateAnonymous();
              sessionLayer->ImportFromString(AL::maya::utils::convert(serializedSessionLayer));
            }
          }
        }
      AL_END_PROFILE_SECTION();

      AL_BEGIN_PROFILE_SECTION(OpenRootLayer);

      // Initialise the asset resolver
      PXR_NS::ArGetResolver().ConfigureResolverForAsset(fileString);

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
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::loadStage is called with extra session layer.\n");
          m_stage = UsdStage::OpenMasked(rootLayer, sessionLayer, mask, loadOperation);
        }
        else
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::loadStage is called without any session layer.\n");
          m_stage = UsdStage::OpenMasked(rootLayer, mask, loadOperation);
        }

        // Expand the mask, since we do not really want to mask the possible relation targets.
        m_stage->ExpandPopulationMask();

        // Save the initial edit target
        trackEditTargetLayer();

        AL_END_PROFILE_SECTION();
      }
      else
      {
        // file path not valid
        if(file.length())
        {
          TF_DEBUG(ALUSDMAYA_TRANSLATORS).Msg("ProxyShape::loadStage failed to open the usd file: %s.\n", file.asChar());
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
    m_path = SdfPath(AL::maya::utils::convert(primPathStr));
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

  if(m_stage && !MFileIO::isReadingFile())
  {
    AL_BEGIN_PROFILE_SECTION(PostLoadProcess);
      // execute the post load process to import any custom prims
      cmds::ProxyShapePostLoadProcess::initialise(this);
      findTaggedPrims();
    AL_END_PROFILE_SECTION();
  }

  AL_END_PROFILE_SECTION();

  if(MGlobal::kInteractive == MGlobal::mayaState())
  {
    std::stringstream strstr;
    strstr << "Breakdown for file: " << file << std::endl;
    AL::usdmaya::Profiler::printReport(strstr);
    MGlobal::displayInfo(AL::maya::utils::convert(strstr.str()));
  }

  stageDataDirtyPlug().setValue(true);
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::updateLockPrims(const SdfPathSet& lockTransformPrims, const SdfPathSet& lockInheritedPrims,
                                 const SdfPathSet& unlockedPrims)
{
  bool lockChanged = false;
  for (auto lock : lockTransformPrims)
  {
    auto inserted = m_lockTransformPrims.insert(lock);
    lockChanged |= inserted.second;
    auto erased = m_lockInheritedPrims.erase(lock);
    lockChanged |= erased;
  }
  for (auto inherited : lockInheritedPrims)
  {
    auto erased = m_lockTransformPrims.erase(inherited);
    lockChanged |= erased;
    auto inserted = m_lockInheritedPrims.insert(inherited);
    lockChanged |= inserted.second;
  }
  for (auto unlocked : unlockedPrims)
  {
    auto erased = m_lockTransformPrims.erase(unlocked);
    lockChanged |= erased;
    erased = m_lockInheritedPrims.erase(unlocked);
    lockChanged |= erased;
  }
  return lockChanged;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructExcludedPrims()
{
  m_excludedGeometry = getExcludePrimPaths();
  constructGLImagingEngine();
}

//----------------------------------------------------------------------------------------------------------------------
bool ProxyShape::lockTransformAttribute(const SdfPath& path, const bool lock)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::lockTransformAttribute\n");

  UsdPrim prim = m_stage->GetPrimAtPath(path);
  if(!prim.IsValid())
  {
    TF_DEBUG_MSG(ALUSDMAYA_EVALUATION,"ProxyShape::lockTransformAttribute prim path not valid '%s'\n", prim.GetPath().GetString().c_str());
    return false;
  }


  VtValue mayaPath = prim.GetCustomDataByKey(TfToken("MayaPath"));
  MObject lockObject;
  if (!mayaPath.IsEmpty())
  {
    MString pathStr = AL::maya::utils::convert(mayaPath.Get<std::string>());
    MSelectionList sl;
    MObject selObj;
    if (sl.add(pathStr) == MStatus::kSuccess)
    {
      sl.getDependNode(0, selObj);
    }
    if (selObj.hasFn(MFn::kTransform))
    {
      lockObject = selObj;
    }
  }
  else
  {
    std::vector<MObjectHandle> objHdls;
    context()->getMObjects(path, objHdls);
    for (auto objHdl : objHdls)
    {
      if (objHdl.isValid() && objHdl.object().hasFn(MFn::kTransform))
      {
        lockObject == objHdl.object();
        break;
      }
    }
  }

  if (lockObject.isNull())
    return false;


  MPlug t(lockObject, m_transformTranslate);
  MPlug r(lockObject, m_transformRotate);
  MPlug s(lockObject, m_transformScale);

  t.setLocked(lock);
  r.setLocked(lock);
  s.setLocked(lock);

  if (lock && MFnDependencyNode(lockObject).typeId() == AL_USDMAYA_TRANSFORM)
  {
    MPlug(lockObject, Transform::pushToPrim()).setBool(false);
  }
  TF_DEBUG_MSG(ALUSDMAYA_EVALUATION,"ProxyShape::lockTransformAttribute Setting lock for '%s'\n", prim.GetPath().GetString().c_str());
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::constructLockPrims()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::constructLockPrims\n");
  SdfPathSet primsNeedLock = m_lockTransformPrims;

  // add inherited lock prims if their parents are already in.
  for (auto inherited : m_lockInheritedPrims)
  {
    const SdfPath parentPath = inherited.GetParentPath();
    if (parentPath.IsEmpty())
      continue;
    auto parentIter = primsNeedLock.find(parentPath);
    if (parentIter != primsNeedLock.end())
    {
      auto lowerIter = std::lower_bound(parentIter, primsNeedLock.end(), inherited);
      primsNeedLock.insert(lowerIter, inherited);
    }
  }

  SdfPathVector primsToLock;
  primsToLock.reserve(primsNeedLock.size());
  SdfPathVector primsToUnlock;
  primsToUnlock.reserve(m_currentLockedPrims.size());
  std::set_difference(primsNeedLock.begin(), primsNeedLock.end(), m_currentLockedPrims.begin(),
                      m_currentLockedPrims.end(), std::back_inserter(primsToLock));
  std::set_difference(m_currentLockedPrims.begin(), m_currentLockedPrims.end(), primsNeedLock.begin(),
                      primsNeedLock.end(), std::back_inserter(primsToUnlock));

  for (auto lock : primsToLock)
  {
    if (lockTransformAttribute(lock, true))
    {
      m_currentLockedPrims.insert(lock);
    }
  }
  for (auto unlock : primsToUnlock)
  {
    if (lockTransformAttribute(unlock, false))
    {
      m_currentLockedPrims.erase(unlock);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::onAttributeChanged(MNodeMessage::AttributeMessage msg, MPlug& plug, MPlug&, void* clientData)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::onAttributeChanged\n");

  const SdfPath rootPath(std::string("/"));
  ProxyShape* proxy = (ProxyShape*)clientData;
  if(msg & MNodeMessage::kAttributeSet)
  {
    // Delay stage creation if opening a file, because we haven't created the LayerManager node yet
    if(plug == m_filePath)
    {
      if (MFileIO::isReadingFile())
      {
        m_unloadedProxyShapes.push_back(MObjectHandle(proxy->thisMObject()));
      }
      else
      {
        proxy->loadStage();
      }
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
          proxy->m_path = SdfPath(AL::maya::utils::convert(primPathStr));
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
void ProxyShape::removeAttributeChangedCallback()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::removeAttributeChangedCallback\n");
  if(m_attributeChanged != -1)
  {
    MMessage::removeCallback(m_attributeChanged);
    m_attributeChanged = -1;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::addAttributeChangedCallback()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::addAttributeChangedCallback\n");
  if(m_attributeChanged == -1)
  {
    MObject obj = thisMObject();
    m_attributeChanged = MNodeMessage::addAttributeChangedCallback(obj, onAttributeChanged, (void*)this);
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::postConstructor()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::postConstructor\n");
  setRenderable(true);
  addAttributeChangedCallback();
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

void ProxyShape::findTaggedPrims()
{
  findTaggedPrims(m_hierarchyIterationLogics);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::findTaggedPrims(const HierarchyIterationLogics& iterationLogics)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::iteratePrimHierarchy\n");
  if(!m_stage)
    return;

  for(auto hl : iterationLogics)
  {
    hl->preIteration();
  }

  MDagPath m_parentPath;
  for(fileio::TransformIterator it(m_stage, m_parentPath); !it.done(); it.next())
  {
    const UsdPrim& prim = it.prim();
    if(!prim.IsValid())
      continue;

    for(auto hl : iterationLogics)
    {
      hl->iteration(it, prim);
    }
  }

  for(auto hl : iterationLogics)
  {
    hl->postIteration();
  }
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::findExcludedGeometry()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::findExcludedGeometry\n");
  if(!m_stage)
    return;

  m_findExcludedPrims.preIteration();
  MDagPath m_parentPath;

  for(fileio::TransformIterator it(m_stage, m_parentPath); !it.done(); it.next())
  {
    const UsdPrim& prim = it.prim();
    if(!prim.IsValid())
      continue;
    m_findExcludedPrims.iteration(it, prim);
  }

  m_findExcludedPrims.postIteration();
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::findSelectablePrims()
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::findSelectablePrims\n");
  if(!m_stage)
    return;

  m_findUnselectablePrims.preIteration();

  MDagPath m_parentPath;
  for(fileio::TransformIterator it(m_stage, m_parentPath); !it.done(); it.next())
  {
    const UsdPrim& prim = it.prim();
    if(!prim.IsValid())
      continue;

    m_findUnselectablePrims.iteration(it, prim);
  }

  m_findUnselectablePrims.postIteration();
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
            MFnReference mfnRef(temp);
            MString referenceFilename = mfnRef.fileName(
                true /*resolvedName*/,
                true /*includePath*/,
                true /*includeCopyNumber*/);
            TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("ProxyShape::unloadMayaReferences unloading %s\n", referenceFilename.asChar());
            MFileIO::unloadReferenceByNode(temp);
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

    if (!drivenTransforms.drivenPrimPaths().empty())
    {
      if(!drivenTransforms.update(m_stage, currentTime))
      {
        MString command("failed to update driven prims on block: ");
        MGlobal::displayError(command + elemIdx);
      }
    }
  }
  return dataBlock.setClean(plug);
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::serialiseTransformRefs()
{
  triggerEvent("PreSerialiseTransformRefs");

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

  triggerEvent("PostSerialiseTransformRefs");
}

//----------------------------------------------------------------------------------------------------------------------
void ProxyShape::deserialiseTransformRefs()
{
  triggerEvent("PreDeserialiseTransformRefs");

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

  triggerEvent("PostDeserialiseTransformRefs");
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
void ProxyShape::registerEvents()
{
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
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
