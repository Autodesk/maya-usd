//
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#include "mayaHydraSceneIndex.h"

#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnComponent.h>
#include <maya/MFnMesh.h>
#include <maya/MItDag.h>
#include <maya/MMatrixArray.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MProfiler.h>
#include <maya/MSelectionList.h>
#include <maya/MShaderManager.h>
#include <maya/MString.h>

#include <mayaHydraLib/adapters/adapterRegistry.h>
#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/hydraUtils.h>
#include <mayaHydraLib/mayaHydra.h>
#include <mayaHydraLib/mayaUtils.h>
#include <mayaHydraLib/mixedUtils.h>
#include <mayaHydraLib/sceneIndex/mayaHydraDataSource.h>

#include <pxr/base/tf/envSetting.h>
#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include "pxr/imaging/hd/dirtyBitsTranslator.h"
#include <pxr/imaging/hd/rprim.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

PXR_NAMESPACE_OPEN_SCOPE
// Bring the MayaHydra namespace into scope.
// The following code currently lives inside the pxr namespace, but it would make more sense to 
// have it inside the MayaHydra namespace. This using statement allows us to use MayaHydra symbols
// from within the pxr namespace as if we were in the MayaHydra namespace.
// Remove this once the code has been moved to the MayaHydra namespace.
using namespace MayaHydra;

TF_DEFINE_ENV_SETTING(MAYA_HYDRA_USE_MESH_ADAPTER, false,
    "Use mesh adapter instead of MRenderItem for Maya meshes.");

bool useMeshAdapter() {
    static bool uma = TfGetEnvSetting(MAYA_HYDRA_USE_MESH_ADAPTER);
    return uma;
}

namespace {
    bool filterMesh(const MRenderItem& ri) {
        return useMeshAdapter() ?
            // Filter our mesh render items, and let the mesh adapter handle Maya
            // meshes.  The MRenderItem::name() for meshes is "StandardShadedItem", 
            // their MRenderItem::type() is InternalMaterialItem, but 
            // this type can also be used for other purposes, e.g. face groups, so
            // using the name is more appropriate.
            (ri.name() == "StandardShadedItem") : false;
    }

    bool isRenderItem_aiSkyDomeLightTriangleShape(const MRenderItem& renderItem)
    {
        static const std::string _aiSkyDomeLight("aiSkyDomeLight");

        const auto prim = renderItem.primitive();
        MDagPath dag = renderItem.sourceDagPath();
        if (dag.isValid() && (MHWRender::MGeometry::Primitive::kTriangles == prim) && (MHWRender::MRenderItem::DecorationItem == renderItem.type())) {
            std::string fpName = dag.fullPathName().asChar();
            if (fpName.find(_aiSkyDomeLight) != std::string::npos) {
                //This render item is a aiSkyDomeLight
                return true;
            }
        }

        return false;
    }

    bool AreLightsParamsWeUseDifferent(const GlfSimpleLight& light1, const GlfSimpleLight& light2)
    {
        // We only update 3 parameters in the default light : position, diffuse and specular. We don't
        // use the primitive's transform.
        return (light1.GetPosition() != light2.GetPosition())
            || // Position (in which we actually store a direction, updated when rotating the view for
               // example)
            (light1.GetDiffuse() != light2.GetDiffuse())
            || (light1.GetSpecular() != light2.GetSpecular());
    }

    static const SdfPath lightedObjectsPath = SdfPath(std::string("Lighted"));

    template<class T> SdfPath toSdfPath(const T& src);
    template<> inline SdfPath toSdfPath<MDagPath>(const MDagPath& dag) {
        return DagPathToSdfPath(dag, false, false);
    }
    template<> inline SdfPath toSdfPath<MRenderItem>(const MRenderItem& ri) {
        return RenderItemToSdfPath(ri, false);
    }

    template<class T> SdfPath maybePrepend(const T& src, const SdfPath& inPath);
    template<> inline SdfPath maybePrepend<MDagPath>(
        const MDagPath&, const SdfPath& inPath
        ) {
        return inPath;
    }
    template<> inline SdfPath maybePrepend<MRenderItem>(
        const MRenderItem& ri, const SdfPath& inPath
        ) {
        // Prepend Maya node name, for organisation and readability.
        std::string dependNodeNameString(MFnDependencyNode(ri.sourceDagPath().node()).name().asChar());
        SanitizeNameForSdfPath(dependNodeNameString);
        return SdfPath(dependNodeNameString).AppendPath(inPath);
    }

    ///Returns false if this object should not be lighted, true if it should be lighted
    template<class T> bool shouldBeLighted(const T& src);
    //Template specialization for MDagPath
    template<> inline bool shouldBeLighted<MDagPath>(const MDagPath& dag) {
        return (MFnDependencyNode(dag.node()).typeName().asChar() == TfToken("mesh"));
    }
    //Template specialization for MRenderItem
    template<> inline bool shouldBeLighted<MRenderItem>(const MRenderItem& ri) {

        //Special case to recognize the Arnold skydome light
        if (isRenderItem_aiSkyDomeLightTriangleShape(ri)) {
            return false;//Don't light the sky dome light shape
        }

        return (MHWRender::MGeometry::Primitive::kLines != ri.primitive()
            && MHWRender::MGeometry::Primitive::kLineStrip != ri.primitive()
            && MHWRender::MGeometry::Primitive::kPoints != ri.primitive());
    }

    template<class T>
    SdfPath GetMayaPrimPath(const T& src)
    {
        SdfPath mayaPath = toSdfPath(src);
        if (mayaPath.IsEmpty() || mayaPath.IsAbsoluteRootPath())
            return {};

        // We cannot append an absolute path (I.e : starting with "/")
        if (mayaPath.IsAbsolutePath()) {
            mayaPath = mayaPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
        }

        mayaPath = maybePrepend(src, mayaPath);

        if (shouldBeLighted(src)) {
            // Use a specific prefix when it's not an object that needs to interact with lights and shadows to be able
            // We filter the objects that don't have this prefix in lights HdLightTokens->shadowCollection parameter
            mayaPath = lightedObjectsPath.AppendPath(mayaPath);
        }

        return mayaPath;
    }

    SdfPath _GetPrimPath(const SdfPath& base, const MDagPath& dg)
    {
        return base.AppendPath(GetMayaPrimPath(dg));
    }

    SdfPath GetRenderItemMayaPrimPath(const MRenderItem& ri)
    {
        if (ri.InternalObjectId() == 0)
            return {};

        return GetMayaPrimPath(ri);
    }

    SdfPath GetRenderItemPrimPath(const SdfPath& base, const MRenderItem& ri)
    {
        return base.AppendPath(GetRenderItemMayaPrimPath(ri));
    }

    template <typename T, typename F> inline void _MapAdapter(F)
    {
        // Do nothing.
    }

    template <typename T, typename M0, typename F, typename... M>
    inline void _MapAdapter(F f, const M0& m0, const M&... m)
    {
        for (auto& it : m0) {
            f(static_cast<T*>(it.second.get()));
        }
        _MapAdapter<T>(f, m...);
    }

    template <typename T, typename F> inline bool _FindAdapter(const SdfPath&, F) { return false; }

    template <typename T, typename M0, typename F, typename... M>
    inline bool _FindAdapter(const SdfPath& id, F f, const M0& m0, const M&... m)
    {
        auto* adapterPtr = TfMapLookupPtr(m0, id);
        if (adapterPtr == nullptr) {
            return _FindAdapter<T>(id, f, m...);
        }
        else {
            f(static_cast<T*>(adapterPtr->get()));
            return true;
        }
    }

    template <typename T, typename F> inline bool _RemoveAdapter(const SdfPath&, F) { return false; }

    template <typename T, typename M0, typename F, typename... M>
    inline bool _RemoveAdapter(const SdfPath& id, F f, M0& m0, M&... m)
    {
        auto* adapterPtr = TfMapLookupPtr(m0, id);
        if (adapterPtr == nullptr) {
            return _RemoveAdapter<T>(id, f, m...);
        }
        else {
            f(static_cast<T*>(adapterPtr->get()));
            m0.erase(id);
            return true;
        }
    }

    template <typename R> inline R _GetDefaultValue() { return {}; }

    template <typename T, typename R, typename F> inline R _GetValue(const SdfPath&, F)
    {
        return _GetDefaultValue<R>();
    }

    template <typename T, typename R, typename F, typename M0, typename... M>
    inline R _GetValue(const SdfPath& id, F f, const M0& m0, const M&... m)
    {
        auto* adapterPtr = TfMapLookupPtr(m0, id);
        if (adapterPtr == nullptr) {
            return _GetValue<T, R>(id, f, m...);
        }
        else {
            return f(static_cast<T*>(adapterPtr->get()));
        }
    }

    void _onDagNodeAdded(MObject& obj, void* clientData)
    {
        reinterpret_cast<MayaHydraSceneIndex*>(clientData)->OnDagNodeAdded(obj);
    }

    void _onDagNodeRemoved(MObject& obj, void* clientData)
    {
        reinterpret_cast<MayaHydraSceneIndex*>(clientData)->OnDagNodeRemoved(obj);
    }

    const MString defaultLightSet("defaultLightSet");

    void _connectionChanged(MPlug& srcPlug, MPlug& destPlug, bool made, void* clientData)
    {
        TF_UNUSED(made);
        const auto srcObj = srcPlug.node();
        if (!srcObj.hasFn(MFn::kTransform)) {
            return;
        }
        const auto destObj = destPlug.node();
        if (!destObj.hasFn(MFn::kSet)) {
            return;
        }
        if (srcPlug != MayaAttrs::dagNode::instObjGroups) {
            return;
        }
        MStatus           status;
        MFnDependencyNode destNode(destObj, &status);
        if (ARCH_UNLIKELY(!status)) {
            return;
        }
        if (destNode.name() != defaultLightSet) {
            return;
        }
        auto* index = reinterpret_cast<MayaHydraSceneIndex*>(clientData);
        MDagPath dag;
        status = MDagPath::getAPathTo(srcObj, dag);
        if (ARCH_UNLIKELY(!status)) {
            return;
        }
        unsigned int shapesBelow = 0;
        dag.numberOfShapesDirectlyBelow(shapesBelow);
        for (auto i = decltype(shapesBelow) { 0 }; i < shapesBelow; ++i) {
            auto dagCopy = dag;
            dagCopy.extendToShapeDirectlyBelow(i);
            index->UpdateLightVisibility(dagCopy);
        }
    }

    SdfPath _GetMaterialPath(const SdfPath& base, const MObject& obj)
    {
        MStatus           status;
        MFnDependencyNode node(obj, &status);
        if (!status) {
            return {};
        }
        const auto* chr = node.name().asChar();
        if (chr == nullptr || chr[0] == '\0') {
            return {};
        }

        std::string nodeName(chr);
        SanitizeNameForSdfPath(nodeName);
        return base.AppendPath(SdfPath(nodeName));
    }

    bool GetShadingEngineNode(const MRenderItem& ri, MObject& shadingEngineNode)
    {
        MDagPath dagPath = ri.sourceDagPath();
        if (dagPath.isValid()) {
            MFnDagNode   dagNode(dagPath.node());
            MObjectArray sets, comps;
            dagNode.getConnectedSetsAndMembers(dagPath.instanceNumber(), sets, comps, true);
            assert(sets.length() == comps.length());
            for (uint32_t i = 0; i < sets.length(); ++i) {
                const MObject& object = sets[i];
                if (object.apiType() == MFn::kShadingEngine) {
                    // To support per-face shading, find the shading node matched with the render item
                    const MObject& comp = comps[i];
                    MObject        shadingComp = ri.shadingComponent();
                    if (shadingComp.isNull() || comp.isNull()
                        || MFnComponent(comp).isEqual(shadingComp)) {
                        shadingEngineNode = object;
                        return true;
                    }
                }
            }
        }
        return false;
    }
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((MayaDefaultMaterial, "__maya_default_material__"))
    (diffuseColor)
    (emissiveColor)
    (roughness)
    (MayaHydraMeshPoints)
    (constantLighting)
    (DefaultMayaLight)
);

SdfPath MayaHydraSceneIndex::_fallbackMaterial;
SdfPath MayaHydraSceneIndex::_mayaDefaultMaterialPath; // Common to all scene indexes
VtValue MayaHydraSceneIndex::_mayaDefaultMaterial;
SdfPath MayaHydraSceneIndex::_mayaDefaultLightPath; // Common to all scene indexes

MayaHydraSceneIndex::MayaHydraSceneIndex(
    SdfPath& id,
    MayaHydraDelegate::InitData& initData,
    bool lightEnabled)
    : _ID(initData.delegateID)
    , _producer(initData.producer)
    , _renderIndex(initData.renderIndex)
    , _isHdSt(initData.isHdSt)
    , _rprimPath(initData.delegateID.AppendPath(SdfPath(std::string("rprims"))))
    , _sprimPath(initData.delegateID.AppendPath(SdfPath(std::string("sprims"))))
    , _materialPath(initData.delegateID.AppendPath(SdfPath(std::string("materials"))))
{
    static std::once_flag once;
    std::call_once(once, []() {
        _mayaDefaultMaterialPath = SdfPath::AbsoluteRootPath().AppendChild(
            _tokens->MayaDefaultMaterial); // Is an absolute path, not linked to a scene index
        _mayaDefaultLightPath = SdfPath::AbsoluteRootPath().AppendChild(_tokens->DefaultMayaLight);
        _mayaDefaultMaterial = MayaHydraSceneIndex::CreateMayaDefaultMaterial();
        _fallbackMaterial = SdfPath::EmptyPath(); // Empty path for hydra fallback material
    });
}

MayaHydraSceneIndex::~MayaHydraSceneIndex()
{
    for (auto callback : _callbacks) {
        MMessage::removeCallback(callback);
    }
    _MapAdapter<MayaHydraAdapter>(
        [](MayaHydraAdapter* a) { a->RemoveCallbacks(); },
        _renderItemsAdapters,
        _shapeAdapters,
        _lightAdapters,
        _materialAdapters);

    SetDefaultLightEnabled(false);
}

void MayaHydraSceneIndex::HandleCompleteViewportScene(const MDataServerOperation::MViewportScene& scene, MFrameContext::DisplayStyle ds)
{
    const bool playbackRunning = MAnimControl::isPlaying();

    if (_isPlaybackRunning != playbackRunning) {
        // The value has changed, we are calling SetPlaybackChanged so that every render item that
        // has its visibility dependent on the playback should dirty its hydra visibility flag so
        // its gets recomputed.
        for (auto it = _renderItemsAdapters.begin(); it != _renderItemsAdapters.end(); it++) {
            it->second->SetPlaybackChanged();
        }

        _isPlaybackRunning = playbackRunning;
    }

    // First loop to get rid of removed items
    constexpr int kInvalidId = 0;
    for (size_t i = 0; i < scene.mRemovalCount; i++) {
        int fastId = scene.mRemovals[i];
        if (fastId == kInvalidId)
            continue;
        MayaHydraRenderItemAdapterPtr ria = nullptr;
        if (_GetRenderItem(fastId, ria)) {
            _RemoveRenderItem(ria);
        }
        assert(ria != nullptr);
    }

    // My version, does minimal update
    // This loop could, in theory, be parallelized.  Unclear how large the gains would be, but maybe
    // nothing to lose unless there is some internal contention in USD.
    for (size_t i = 0; i < scene.mCount; i++) {
        auto flags = scene.mFlags[i];
        if (flags == 0) {
            continue;
        }

        auto& ri = *scene.mItems[i];

        // Meshes can optionally be handled by the mesh adapter, rather than by
        // render items.
        if (filterMesh(ri)) {
            continue;
        }

        int                           fastId = ri.InternalObjectId();
        MayaHydraRenderItemAdapterPtr ria = nullptr;
        if (!_GetRenderItem(fastId, ria)) {
            const SdfPath slowId = _GetRenderItemPrimPath(ri);
            if (slowId.IsEmpty()) {
                continue;
            }
            // MAYA-128021: We do not currently support maya instances.
            MDagPath dagPath(ri.sourceDagPath());
            ria = std::make_shared<MayaHydraRenderItemAdapter>(dagPath, slowId, fastId, _producer, ri);

            //Update the render item adapter if this render item is an aiSkydomeLight shape
            ria->SetIsRenderITemAnaiSkydomeLightTriangleShape(isRenderItem_aiSkyDomeLightTriangleShape(ri));

            _AddRenderItem(ria);
        }

        SdfPath material;
        MObject shadingEngineNode;
        if (!_GetRenderItemMaterial(ri, material, shadingEngineNode)) {
            if (material != kInvalidMaterial) {
                _CreateMaterial(material, shadingEngineNode);
            }
        }

        if (flags & MDataServerOperation::MViewportScene::MVS_changedEffect) {
            ria->SetMaterial(material);
        }

        MColor                   wireframeColor;
        MHWRender::DisplayStatus displayStatus = MHWRender::kNoStatus;

        MDagPath dagPath = ri.sourceDagPath();
        if (dagPath.isValid()) {
            wireframeColor = MGeometryUtilities::wireframeColor(
                dagPath); // This is a color managed VP2 color, it will need to be unmanaged at some
            // point
            displayStatus = MGeometryUtilities::displayStatus(dagPath);
        }

        const MayaHydraRenderItemAdapter::UpdateFromDeltaData data(
            ri, flags, wireframeColor, displayStatus);
        ria->UpdateFromDelta(data);
        if (flags & MDataServerOperation::MViewportScene::MVS_changedMatrix) {
            ria->UpdateTransform(ri);
        }
    }
}

#if PXR_VERSION < 2308
void MayaHydraSceneIndex::AddPrims(const AddedPrimEntries& entries)
{
    HdSceneIndexObserver::AddedPrimEntries observerEntries;
    observerEntries.reserve(entries.size());

    for (const AddedPrimEntry& entry : entries) {
        observerEntries.emplace_back(entry.primPath, entry.primType);
        _entries[entry.primPath] = { entry.primType, entry.dataSource };
    }

    _SendPrimsAdded(observerEntries);
}

void MayaHydraSceneIndex::RemovePrims( const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
        _entries.erase(entry.primPath);
    }

    _SendPrimsRemoved(entries);
}

void MayaHydraSceneIndex::DirtyPrims(const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    // NOTE: Filtering the DirtyPrims notices to include only paths which are
    //       present in the internal table. This is (currently) useful as
    //       front-end emulation makes use of an HdRetainedSceneIndex in order
    //       to transfer population and value queries into a scene index. The
    //       current implementation shares a single render index and some
    //       emulated actions cause all prims to be dirtied -- which can
    //       include prims not within this scene index.
    //
    //       This filtering behavior may still be desired independent of the
    //       emulation case which inspired it.
    HdSceneIndexObserver::DirtiedPrimEntries observerEntries;
    observerEntries.reserve(entries.size());
    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        if (_entries.find(entry.primPath) == _entries.end()) {
            continue;
        }
        observerEntries.emplace_back(entry.primPath, entry.dirtyLocators);
    }

    _SendPrimsDirtied(observerEntries);
}

HdSceneIndexPrim MayaHydraSceneIndex::GetPrim(const SdfPath& primPath) const
{
    const auto it = _entries.find(primPath);

    if (it != _entries.end()) {
        return it->second.prim;
    }

    return { TfToken(), nullptr };
}

SdfPathVector MayaHydraSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    SdfPathVector result;

    _PrimEntryTable::const_iterator it = _entries.find(primPath);
    if (it == _entries.end()) {
        return result;
    }

    // increment is depth-first so this will get first child
    ++it;
    while (it != _entries.end()
        && it->first.GetParentPath() == primPath) {

        result.push_back(it->first);

        // we want a sibling so we can't use increment
        it = it.GetNextSubtree();
    }

    return result;
}
#endif

void MayaHydraSceneIndex::Populate()
{
    MayaHydraAdapterRegistry::LoadAllPlugin();

    MStatus status;
    MItDag dagIt(MItDag::kDepthFirst);
    dagIt.traverseUnderWorld(true);
    if (useMeshAdapter()) {
        for (; !dagIt.isDone(); dagIt.next()) {
            MDagPath path;
            dagIt.getPath(path);
            InsertDag(path);
        }
    }
    else {
        for (; !dagIt.isDone(); dagIt.next()) {
            MObject node = dagIt.currentItem(&status);
            if (status != MS::kSuccess)
                continue;
            OnDagNodeAdded(node);
        }
    }

    auto id = MDGMessage::addNodeAddedCallback(_onDagNodeAdded, "dagNode", this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MDGMessage::addNodeRemovedCallback(_onDagNodeRemoved, "dagNode", this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
    id = MDGMessage::addConnectionCallback(_connectionChanged, this, &status);
    if (status) {
        _callbacks.push_back(id);
    }
}

void MayaHydraSceneIndex::SetDefaultLightEnabled(const bool enabled)
{
    if (_useMayaDefaultLight != enabled) {
        _useMayaDefaultLight = enabled;

        if (_useMayaDefaultLight) {
            auto mayaDefaultLightDataSource = MayaHydraDefaultLightDataSource::New(_mayaDefaultLightPath, HdPrimTypeTokens->simpleLight, this);
            AddPrims({ { _mayaDefaultLightPath, HdPrimTypeTokens->simpleLight, mayaDefaultLightDataSource } });
        }
        else
        {
            RemovePrim(_mayaDefaultLightPath);
        }
    }
}

void MayaHydraSceneIndex::SetDefaultLight(const GlfSimpleLight& light)
{
    // We only update 3 parameters in the default light : position (in which we store a direction),
    // diffuse and specular
    // We don't never update the transform for the default light
    const bool lightsParamsWeUseAreDifferent = AreLightsParamsWeUseDifferent(_mayaDefaultLight, light);
    if (lightsParamsWeUseAreDifferent) {
        // Update our light
        _mayaDefaultLight.SetDiffuse(light.GetDiffuse());
        _mayaDefaultLight.SetSpecular(light.GetSpecular());
        _mayaDefaultLight.SetPosition(light.GetPosition());
        MarkPrimDirty(_mayaDefaultLightPath, HdLight::DirtyParams);
    }
}

VtValue MayaHydraSceneIndex::GetMaterialResource(const SdfPath& id)
{
    if (id == _mayaDefaultMaterialPath) {
        return _mayaDefaultMaterial;
    }

    if (id == _fallbackMaterial) {
        return MayaHydraMaterialAdapter::GetPreviewMaterialResource(id);
    }

    auto ret = _GetValue<MayaHydraMaterialAdapter, VtValue>(
        id,
        [](MayaHydraMaterialAdapter* a) -> VtValue { return a->GetMaterialResource(); },
        _materialAdapters);
    return ret.IsEmpty() ? MayaHydraMaterialAdapter::GetPreviewMaterialResource(id) : ret;
}

VtValue MayaHydraSceneIndex::CreateMayaDefaultMaterial()
{
    static const MColor kDefaultGrayColor = MColor(0.5f, 0.5f, 0.5f) * 0.8f;

    HdMaterialNetworkMap networkMap;
    HdMaterialNetwork    network;
    HdMaterialNode       node;
    node.identifier = UsdImagingTokens->UsdPreviewSurface;
    node.path = _mayaDefaultMaterialPath;
    node.parameters.insert(
        { _tokens->diffuseColor,
          VtValue(GfVec3f(kDefaultGrayColor[0], kDefaultGrayColor[1], kDefaultGrayColor[2])) });
    network.nodes.push_back(std::move(node));
    networkMap.map.insert({ HdMaterialTerminalTokens->surface, std::move(network) });
    networkMap.terminals.push_back(_mayaDefaultMaterialPath);
    return VtValue(networkMap);
}

void MayaHydraSceneIndex::PopulateSelectedPaths(
    const MSelectionList& mayaSelection,
    SdfPathVector& selectedSdfPaths,
    const HdSelectionSharedPtr& selection)
{
}

SdfPath MayaHydraSceneIndex::SetCameraViewport(const MDagPath& camPath, const GfVec4d& viewport)
{
    const SdfPath camID = GetPrimPath(camPath, true);
    auto&& cameraAdapter = TfMapLookupPtr(_cameraAdapters, camID);
    if (cameraAdapter) {
        (*cameraAdapter)->SetViewport(viewport);
        return camID;
    }
    return {};
}

bool MayaHydraSceneIndex::AddPickHitToSelectionList(
    const HdxPickHit& hit,
    const MHWRender::MSelectionInfo& selectInfo,
    MSelectionList& selectionList,
    MPointArray& worldSpaceHitPts)
{
    SdfPath hitId = hit.objectId;
    // validate that hit is indeed a maya item. Alternatively, the rprim hit could be an rprim
    // defined by a scene index such as maya usd.
    if (hitId.HasPrefix(GetRprimPath())) {
        _FindAdapter<MayaHydraRenderItemAdapter>(
            hitId,
            [&selectionList, &worldSpaceHitPts, &hit](MayaHydraRenderItemAdapter* a) {
                // prepare the selection path of the hit item, the transform path is expected if available
                const auto& itemPath = a->GetDagPath();
        MDagPath selectPath;
        if (MS::kSuccess != MDagPath::getAPathTo(itemPath.transform(), selectPath)) {
            selectPath = itemPath;
        }
        selectionList.add(selectPath);
        worldSpaceHitPts.append(
            hit.worldSpaceHitPoint[0],
            hit.worldSpaceHitPoint[1],
            hit.worldSpaceHitPoint[2]);
            },
            _renderItemsAdapters);
        return true;
    }

    return false;
}

HdChangeTracker& MayaHydraSceneIndex::GetChangeTracker()
{
    return _renderIndex->GetChangeTracker();
}

SdfPath MayaHydraSceneIndex::GetDelegateID(TfToken name)
{
    return _ID;
}

void MayaHydraSceneIndex::PreFrame(const MHWRender::MDrawContext& context)
{
    bool useDefaultMaterial
        = (context.getDisplayStyle() & MHWRender::MFrameContext::kDefaultMaterial);
    if (useDefaultMaterial != _useDefaultMaterial) {
        _useDefaultMaterial = useDefaultMaterial;
        if (useMeshAdapter()) {
            for (const auto& shape : _shapeAdapters)
                shape.second->MarkDirty(HdChangeTracker::DirtyMaterialId);
        }
    }

    const bool xRayEnabled = (context.getDisplayStyle() & MHWRender::MFrameContext::kXray);
    if (xRayEnabled != _xRayEnabled) {
        _xRayEnabled = xRayEnabled;
        for (auto& matAdapter : _materialAdapters)
            matAdapter.second->EnableXRayShadingMode(_xRayEnabled);
    }

    if (!_materialTagsChanged.empty()) {
        if (IsHdSt()) {
            for (const auto& id : _materialTagsChanged) {
                if (_GetValue<MayaHydraMaterialAdapter, bool>(
                    id,
                    [](MayaHydraMaterialAdapter* a) { return a->UpdateMaterialTag(); },
                    _materialAdapters)) {
                    auto& renderIndex = GetRenderIndex();
                    for (const auto& rprimId : renderIndex.GetRprimIds()) {
                        const auto* rprim = renderIndex.GetRprim(rprimId);
                        if (rprim != nullptr && rprim->GetMaterialId() == id) {
                            RebuildAdapterOnIdle(
                                rprim->GetId(), MayaHydraSceneIndex::RebuildFlagPrim);
                        }
                    }
                }
            }
        }
        _materialTagsChanged.clear();
    }

    if (!_lightsToAdd.empty()) {
        for (auto& lightToAdd : _lightsToAdd) {
            MDagPath dag;
            MStatus  status = MDagPath::getAPathTo(lightToAdd.first, dag);
            if (!status) {
                return;
            }
            CreateLightAdapter(dag);
        }
        _lightsToAdd.clear();
    }

    if (useMeshAdapter() && !_addedNodes.empty()) {
        for (const auto& obj : _addedNodes) {
            if (obj.isNull()) {
                continue;
            }
            MDagPath dag;
            MStatus  status = MDagPath::getAPathTo(obj, dag);
            if (!status) {
                return;
            }
            // We need to check if there is an instanced shape below this dag
            // and insert it as well, because they won't be inserted.
            if (dag.hasFn(MFn::kTransform)) {
                const auto childCount = dag.childCount();
                for (auto child = decltype(childCount) { 0 }; child < childCount; ++child) {
                    auto dagCopy = dag;
                    dagCopy.push(dag.child(child));
                    if (dagCopy.isInstanced() && dagCopy.instanceNumber() > 0) {
                        AddNewInstance(dagCopy);
                    }
                }
            }
            else {
                InsertDag(dag);
            }
        }
        _addedNodes.clear();
    }

    // We don't need to rebuild something that's already being recreated.
    // Since we have a few elements, linear search over vectors is going to
    // be okay.
    if (!_adaptersToRecreate.empty()) {
        for (const auto& it : _adaptersToRecreate) {
            RecreateAdapter(std::get<0>(it), std::get<1>(it));
            for (auto itr = _adaptersToRebuild.begin(); itr != _adaptersToRebuild.end(); ++itr) {
                if (std::get<0>(it) == std::get<0>(*itr)) {
                    _adaptersToRebuild.erase(itr);
                    break;
                }
            }
        }
        _adaptersToRecreate.clear();
    }
    if (!_adaptersToRebuild.empty()) {
        for (const auto& it : _adaptersToRebuild) {
            _FindAdapter<MayaHydraAdapter>(
                std::get<0>(it),
                [&](MayaHydraAdapter* a) {
                    if (std::get<1>(it) & MayaHydraSceneIndex::RebuildFlagCallbacks) {
                        a->RemoveCallbacks();
                        a->CreateCallbacks();
                    }
            if (std::get<1>(it) & MayaHydraSceneIndex::RebuildFlagPrim) {
                a->RemovePrim();
                a->Populate();
            }
                },
                _shapeAdapters,
                    _lightAdapters,
                    _materialAdapters);
        }
        _adaptersToRebuild.clear();
    }
    if (!IsHdSt()) {
        return;
    }

    // Some 3rd parties lights may be ignored by the call to
    // MHWRender::MDrawContext::numberOfActiveLights (like the Arnold lights which are seen by Maya
    // as locators)
    std::vector<MDagPath> activelightPaths
        = _arnoldLightPaths; // We suppose the Arnold lights are always active

    constexpr auto considerAllSceneLights = MHWRender::MDrawContext::kFilteredIgnoreLightLimit;
    MStatus        status;
    const auto     numLights = context.numberOfActiveLights(considerAllSceneLights, &status);

    if ((!status || numLights == 0) && (0 == activelightPaths.size())) {
        _MapAdapter<MayaHydraLightAdapter>(
            [](MayaHydraLightAdapter* a) { a->SetLightingOn(false); },
            _lightAdapters); // Turn off all lights
        return;
    }

    MIntArray intVals;
    MMatrix   matrixVal;
    for (auto i = decltype(numLights) { 0 }; i < numLights; ++i) {
        auto* lightParam = context.getLightParameterInformation(i, considerAllSceneLights);
        if (lightParam == nullptr) {
            continue;
        }
        const auto lightPath = lightParam->lightPath();
        if (!lightPath.isValid()) {
            continue;
        }
        if (IsUfeItemFromMayaUsd(lightPath)) {
            // If this is a UFE light created by maya-usd, it will have already added it to Hydra
            continue;
        }

        activelightPaths.push_back(lightPath);

        if (!lightParam->getParameter(MHWRender::MLightParameterInformation::kShadowOn, intVals)
            || intVals.length() < 1 || intVals[0] != 1) {
            continue;
        }

        if (lightParam->getParameter(
            MHWRender::MLightParameterInformation::kShadowViewProj, matrixVal)) {
            _FindAdapter<MayaHydraLightAdapter>(
                GetPrimPath(lightPath, true),
                [&matrixVal](MayaHydraLightAdapter* a) {
                    a->SetShadowProjectionMatrix(GetGfMatrixFromMaya(matrixVal));
                },
                _lightAdapters);
        }
    }

    // Turn on active lights, turn off non-active lights, and add non-created active lights.
    _MapAdapter<MayaHydraLightAdapter>(
        [&](MayaHydraLightAdapter* a) {
            auto it = std::find(activelightPaths.begin(), activelightPaths.end(), a->GetDagPath());
    if (it != activelightPaths.end()) {
        a->SetLightingOn(true);
        activelightPaths.erase(it);
    }
    else {
        a->SetLightingOn(false);
    }
        },
        _lightAdapters);
    for (const auto& lightPath : activelightPaths) {
        CreateLightAdapter(lightPath);
    }
}

void MayaHydraSceneIndex::PostFrame()
{
}

void MayaHydraSceneIndex::InsertPrim(
    MayaHydraAdapter* adapter,
    const TfToken& typeId,
    const SdfPath& id)
{
    auto dataSource = MayaHydraDataSource::New(
        id, typeId, this, adapter);

    AddPrims({ { id, typeId, dataSource } });
}

void MayaHydraSceneIndex::MarkPrimDirty(const SdfPath& id, HdDirtyBits dirtyBits)
{
    // Dispatch based on prim type.
    HdSceneIndexPrim prim = GetPrim(id);
    HdDataSourceLocatorSet locators;
    if (HdPrimTypeIsGprim(prim.primType)) {
        HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet(prim.primType, dirtyBits, &locators);
    }
    else {
        HdDirtyBitsTranslator::SprimDirtyBitsToLocatorSet(prim.primType, dirtyBits, &locators);
    }

    if (!locators.IsEmpty()) {
        DirtyPrims({ {id, locators} });
    }
}

void MayaHydraSceneIndex::RemovePrim(const SdfPath& id)
{
    RemovePrims({ id });
}

void MayaHydraSceneIndex::SetParams(const MayaHydraParams& params)
{
    const auto& oldParams = GetParams();
    if (oldParams.displaySmoothMeshes != params.displaySmoothMeshes) {
        // I couldn't find any other way to turn this on / off.
        // I can't convert HdRprim to HdMesh easily and no simple way
        // to get the type of the HdRprim from the render index.
        // If we want to allow creating multiple rprims and returning an id
        // to a subtree, we need to use the HasType function and the mark dirty
        // from each adapter.
        _MapAdapter<MayaHydraRenderItemAdapter>(
            [](MayaHydraRenderItemAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh) || a->HasType(HdPrimTypeTokens->basisCurves)
                || a->HasType(HdPrimTypeTokens->points)) {
                    a->MarkDirty(HdChangeTracker::DirtyTopology);
                }
            },
            _renderItemsAdapters);
        _MapAdapter<MayaHydraDagAdapter>(
            [](MayaHydraDagAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh)) {
                    a->MarkDirty(HdChangeTracker::DirtyTopology);
                }
            },
            _shapeAdapters);
    }
    if (oldParams.motionSampleStart != params.motionSampleStart
        || oldParams.motionSampleEnd != params.motionSampleEnd) {
        _MapAdapter<MayaHydraRenderItemAdapter>(
            [](MayaHydraRenderItemAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh) || a->HasType(HdPrimTypeTokens->basisCurves)
                || a->HasType(HdPrimTypeTokens->points)) {
                    a->InvalidateTransform();
                    a->MarkDirty(HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyTransform);
                }
            },
            _renderItemsAdapters);
        _MapAdapter<MayaHydraDagAdapter>(
            [](MayaHydraDagAdapter* a) {
                if (a->HasType(HdPrimTypeTokens->mesh)) {
                    a->MarkDirty(HdChangeTracker::DirtyPoints);
                }
                else if (a->HasType(HdPrimTypeTokens->camera)) {
                    a->MarkDirty(HdCamera::DirtyParams);
                }
        a->InvalidateTransform();
        a->MarkDirty(HdChangeTracker::DirtyTransform);
            },
            _shapeAdapters,
                _lightAdapters,
                _cameraAdapters);
    }
    // We need to trigger rebuilding shaders.
    if (oldParams.textureMemoryPerTexture != params.textureMemoryPerTexture) {
        _MapAdapter<MayaHydraMaterialAdapter>(
            [](MayaHydraMaterialAdapter* a) { a->MarkDirty(HdMaterial::AllDirty); },
            _materialAdapters);
    }
    if (oldParams.maximumShadowMapResolution != params.maximumShadowMapResolution) {
        _MapAdapter<MayaHydraLightAdapter>(
            [](MayaHydraLightAdapter* a) { a->MarkDirty(HdLight::AllDirty); }, _lightAdapters);
    }

    _params = params;
}

SdfPath MayaHydraSceneIndex::GetMaterialId(const SdfPath& id)
{
    if (_useDefaultMaterial) {
        return _mayaDefaultMaterialPath;
    }

    auto result = TfMapLookupPtr(_renderItemsAdapters, id);
    if (result != nullptr) {
        auto& renderItemAdapter = *result;

        // Check if this render item is a wireframe primitive
        if (MHWRender::MGeometry::Primitive::kLines == renderItemAdapter->GetPrimitive()
            || MHWRender::MGeometry::Primitive::kLineStrip == renderItemAdapter->GetPrimitive()) {
            return _fallbackMaterial;
        }

        auto& material = renderItemAdapter->GetMaterial();

        if (material == kInvalidMaterial) {
            return _fallbackMaterial;
        }

        if (TfMapLookupPtr(_materialAdapters, material) != nullptr) {
            return material;
        }
    }

    if (useMeshAdapter()) {
        auto shapeAdapter = TfMapLookupPtr(_shapeAdapters, id);
        if (shapeAdapter == nullptr) {
            return _fallbackMaterial;
        }
        auto material = shapeAdapter->get()->GetMaterial();
        if (material == MObject::kNullObj) {
            return _fallbackMaterial;
        }
        auto materialId = GetMaterialPath(material);
        if (TfMapLookupPtr(_materialAdapters, materialId) != nullptr) {
            return materialId;
        }

        return _CreateMaterial(materialId, material) ? materialId : _fallbackMaterial;
    }

    return _fallbackMaterial;
}

HdMeshTopology MayaHydraSceneIndex::GetMeshTopology(const SdfPath& id)
{
    return _GetValue<MayaHydraAdapter, HdMeshTopology>(
        id,
        [](MayaHydraAdapter* a) -> HdMeshTopology { return a->GetMeshTopology(); },
        _shapeAdapters,
        _renderItemsAdapters);
}

void MayaHydraSceneIndex::RemoveAdapter(const SdfPath& id)
{
    if (!_RemoveAdapter<MayaHydraAdapter>(
        id,
        [](MayaHydraAdapter* a) {
            a->RemoveCallbacks();
            a->RemovePrim();
        },
        _renderItemsAdapters,
            _shapeAdapters,
            _lightAdapters,
            _materialAdapters)) {
        TF_WARN(
            "MayaHydraSceneIndex::RemoveAdapter(%s) -- Adapter does not exists", id.GetText());
    }
}

void MayaHydraSceneIndex::RecreateAdapterOnIdle(const SdfPath& id, const MObject& obj)
{
    // We expect this to be a small number of objects, so using a simple linear
    // search and a vector is generally a good choice.
    for (auto& it : _adaptersToRecreate) {
        if (std::get<0>(it) == id) {
            std::get<1>(it) = obj;
            return;
        }
    }
    _adaptersToRecreate.emplace_back(id, obj);
}

bool MayaHydraSceneIndex::_GetRenderItem(int fastId, MayaHydraRenderItemAdapterPtr& ria)
{
    // Using SdfPath as the hash table key is extremely slow.  The cost appears to be GetPrimPath,
    // which would depend on MdagPath, which is a wrapper on TdagPath.  TdagPath is a very slow
    // class and best to avoid in any performance- critical area. Simply workaround for the
    // prototype is an additional lookup index based on InternalObjectID.  Long term goal would be
    // that the plug-in rarely, if ever, deals with TdagPath.
    MayaHydraRenderItemAdapterPtr* result = TfMapLookupPtr(_renderItemsAdaptersFast, fastId);

    if (result != nullptr) {
        // adapter already exists, return it
        ria = *result;
        return true;
    }

    return false;
}

void MayaHydraSceneIndex::_AddRenderItem(const MayaHydraRenderItemAdapterPtr& ria)
{
    const SdfPath& primPath = ria->GetID();
    _renderItemsAdaptersFast.insert({ ria->GetFastID(), ria });
    _renderItemsAdapters.insert({ primPath, ria });
}

void MayaHydraSceneIndex::_RemoveRenderItem(const MayaHydraRenderItemAdapterPtr& ria)
{
    const SdfPath& primPath = ria->GetID();
    _renderItemsAdaptersFast.erase(ria->GetFastID());
    _renderItemsAdapters.erase(primPath);
}

bool MayaHydraSceneIndex::_GetRenderItemMaterial(
    const MRenderItem& ri,
    SdfPath& material,
    MObject& shadingEngineNode)
{
    if (MHWRender::MGeometry::Primitive::kLines == ri.primitive()
        || MHWRender::MGeometry::Primitive::kLineStrip == ri.primitive()) {
        material = _fallbackMaterial; // Use fallbackMaterial + constantLighting + displayColor
        return true;
    }

    if (GetShadingEngineNode(ri, shadingEngineNode))
        // Else try to find associated material node if this is a material shader.
        // NOTE: The existing maya material support in hydra expects a shading engine node
    {
        material = GetMaterialPath(shadingEngineNode);
        if (TfMapLookupPtr(_materialAdapters, material) != nullptr) {
            return true;
        }
    }

    return false;
}

SdfPath MayaHydraSceneIndex::_GetRenderItemPrimPath(const MRenderItem& ri)
{
    return GetRenderItemPrimPath(_rprimPath, ri);
}

SdfPath MayaHydraSceneIndex::GetPrimPath(const MDagPath& dg, bool isSprim)
{
    if (isSprim) {
        return _GetPrimPath(_sprimPath, dg);
    }
    else {
        return _GetPrimPath(_rprimPath, dg);
    }
}

GfInterval MayaHydraSceneIndex::GetCurrentTimeSamplingInterval() const
{
    return GfInterval(_params.motionSampleStart, _params.motionSampleEnd);
}

SdfPath MayaHydraSceneIndex::GetLightedPrimsRootPath() const
{
    return _rprimPath.AppendPath(lightedObjectsPath);
}

void MayaHydraSceneIndex::RebuildAdapterOnIdle(const SdfPath& id, uint32_t flags)
{
    // We expect this to be a small number of objects, so using a simple linear
    // search and a vector is generally a good choice.
    for (auto& it : _adaptersToRebuild) {
        if (std::get<0>(it) == id) {
            std::get<1>(it) |= flags;
            return;
        }
    }
    _adaptersToRebuild.emplace_back(id, flags);
}

void MayaHydraSceneIndex::RecreateAdapter(const SdfPath& id, const MObject& obj)
{
    if (_RemoveAdapter<MayaHydraAdapter>(
        id,
        [](MayaHydraAdapter* a) {
            a->RemoveCallbacks();
    a->RemovePrim();
        },
        _lightAdapters)) {
        if (MObjectHandle(obj).isValid()) {
            OnDagNodeAdded(obj);
        }
        return;
    }

    if (useMeshAdapter() && _RemoveAdapter<MayaHydraAdapter>(
        id,
        [](MayaHydraAdapter* a) {
            a->RemoveCallbacks();
            a->RemovePrim();
        },
        _shapeAdapters)) {
        MFnDagNode dgNode(obj);
        MDagPath   path;
        dgNode.getPath(path);
        if (path.isValid() && MObjectHandle(obj).isValid()) {
            InsertDag(path);
        }
        return;
    }

    if (_RemoveAdapter<MayaHydraMaterialAdapter>(
        id,
        [](MayaHydraMaterialAdapter* a) {
            a->RemoveCallbacks();
            a->RemovePrim();
        },
        _materialAdapters)) {
        auto& renderIndex = GetRenderIndex();
        auto& changeTracker = renderIndex.GetChangeTracker();
        for (const auto& rprimId : renderIndex.GetRprimIds()) {
            const auto* rprim = renderIndex.GetRprim(rprimId);
            if (rprim != nullptr && rprim->GetMaterialId() == id) {
                changeTracker.MarkRprimDirty(rprimId, HdChangeTracker::DirtyMaterialId);
            }
        }
        if (MObjectHandle(obj).isValid()) {
            _CreateMaterial(GetMaterialPath(obj), obj);
        }

    }
}


template <typename AdapterPtr, typename Map>
AdapterPtr MayaHydraSceneIndex::_CreateAdapter(
    const MDagPath& dag,
    const std::function<AdapterPtr(MayaHydraSceneProducer*, const MDagPath&)>& adapterCreator,
    Map& adapterMap,
    bool                                                                     isSprim)
{
    // Filter for whether we should even attempt to create the adapter

    if (!adapterCreator) {
        return {};
    }

    if (IsUfeItemFromMayaUsd(dag)) {
        // UFE items that have a Hydra representation will be added to Hydra by maya-usd
        return {};
    }

    // Attempt to create the adapter

    const auto id = GetPrimPath(dag, isSprim);
    if (TfMapLookupPtr(adapterMap, id) != nullptr) {
        return {};
    }
    auto adapter = adapterCreator(GetProducer(), dag);
    if (adapter == nullptr || !adapter->IsSupported()) {
        return {};
    }
    adapter->Populate();
    adapter->CreateCallbacks();
    adapterMap.insert({ id, adapter });
    return adapter;
}

MayaHydraLightAdapterPtr MayaHydraSceneIndex::CreateLightAdapter(const MDagPath& dagPath)
{
    auto lightCreatorFunc = MayaHydraAdapterRegistry::GetLightAdapterCreator(dagPath);
    return _CreateAdapter(dagPath, lightCreatorFunc, _lightAdapters, true);
}

MayaHydraCameraAdapterPtr MayaHydraSceneIndex::CreateCameraAdapter(const MDagPath& dagPath)
{
    auto cameraCreatorFunc = MayaHydraAdapterRegistry::GetCameraAdapterCreator(dagPath);
    return _CreateAdapter(dagPath, cameraCreatorFunc, _cameraAdapters, true);
}

MayaHydraShapeAdapterPtr MayaHydraSceneIndex::CreateShapeAdapter(const MDagPath& dagPath) {
    auto shapeCreatorFunc = MayaHydraAdapterRegistry::GetShapeAdapterCreator(dagPath);
    return _CreateAdapter(dagPath, shapeCreatorFunc, _shapeAdapters);
}

void MayaHydraSceneIndex::OnDagNodeAdded(const MObject& obj)
{
    if (obj.isNull())
        return;

    if (IsUfeItemFromMayaUsd(obj)) {
        // UFE items that have a Hydra representation will be added to Hydra by maya-usd
        return;
    }

    // When not using the mesh adapter we care only about lights for this
    // callback.  It is used to create a LightAdapter when adding a new light
    // in the scene for Hydra rendering.
    if (auto lightFn = MayaHydraAdapterRegistry::GetLightAdapterCreator(obj)) {
        _lightsToAdd.push_back({ obj, lightFn });
    }
    else if (useMeshAdapter()) {
        _addedNodes.push_back(obj);
    }
}

void MayaHydraSceneIndex::OnDagNodeRemoved(const MObject& obj)
{
    const auto it
        = std::remove_if(_lightsToAdd.begin(), _lightsToAdd.end(), [&obj](const auto& item) {
        return item.first == obj;
            });

    if (it != _lightsToAdd.end()) {
        _lightsToAdd.erase(it, _lightsToAdd.end());
    }
    else if (useMeshAdapter()) {
        const auto it = std::remove_if(_addedNodes.begin(), _addedNodes.end(), [&obj](const auto& item) { return item == obj; });

        if (it != _addedNodes.end()) {
            _addedNodes.erase(it, _addedNodes.end());
        }
    }
}

void MayaHydraSceneIndex::InsertDag(const MDagPath& dag)
{
    // We don't care about transforms.
    if (dag.hasFn(MFn::kTransform)) {
        return;
    }

    MFnDagNode dagNode(dag);
    if (dagNode.isIntermediateObject()) {
        return;
    }

    if (IsUfeItemFromMayaUsd(dag)) {
        // UFE items that have a Hydra representation will be added to Hydra by maya-usd
        return;
    }

    // Custom lights don't have MFn::kLight.
    if (GetLightsEnabled()) {
        if (CreateLightAdapter(dag))
            return;
    }
    if (CreateCameraAdapter(dag)) {
        return;
    }
    // We are inserting a single prim and
    // instancer for every instanced mesh.
    if (dag.isInstanced() && dag.instanceNumber() > 0) {
        return;
    }

    auto adapter = CreateShapeAdapter(dag);
    if (adapter) {
        auto material = adapter->GetMaterial();
        if (material != MObject::kNullObj) {
            const auto materialId = GetMaterialPath(material);
            if (TfMapLookupPtr(_materialAdapters, materialId) == nullptr) {
                _CreateMaterial(materialId, material);
            }
        }
    }
}

void MayaHydraSceneIndex::UpdateLightVisibility(const MDagPath& dag)
{
    const auto id = GetPrimPath(dag, true);
    _FindAdapter<MayaHydraLightAdapter>(
        id,
        [](MayaHydraLightAdapter* a) {
            if (a->UpdateVisibility()) {
                a->RemovePrim();
                a->Populate();
                a->InvalidateTransform();
            }
        },
        _lightAdapters);
}

SdfPath MayaHydraSceneIndex::GetMaterialPath(const MObject& obj)
{
    return _GetMaterialPath(_materialPath, obj);
}

bool MayaHydraSceneIndex::_CreateMaterial(const SdfPath& id, const MObject& obj)
{
    TF_DEBUG(MAYAHYDRALIB_ADAPTER_MATERIALS)
        .Msg("MayaHydraSceneIndex::_CreateMaterial(%s)\n", id.GetText());

    auto materialCreator = MayaHydraAdapterRegistry::GetMaterialAdapterCreator(obj);
    if (materialCreator == nullptr) {
        return false;
    }
    auto materialAdapter = materialCreator(id, GetProducer(), obj);
    if (materialAdapter == nullptr || !materialAdapter->IsSupported()) {
        return false;
    }

    if (_xRayEnabled) {
        materialAdapter->EnableXRayShadingMode(_xRayEnabled); // Enable XRay shading mode
    }
    materialAdapter->Populate();
    materialAdapter->CreateCallbacks();
    _materialAdapters.emplace(id, std::move(materialAdapter));
    return true;
}

void MayaHydraSceneIndex::AddNewInstance(const MDagPath& dag)
{
    MDagPathArray dags;
    MDagPath::getAllPathsTo(dag.node(), dags);
    const auto dagsLength = dags.length();
    if (dagsLength == 0) {
        return;
    }
    const auto                             masterDag = dags[0];
    const auto                             id = GetPrimPath(masterDag, false);
    std::shared_ptr<MayaHydraShapeAdapter> masterAdapter;
    if (!TfMapLookup(_shapeAdapters, id, &masterAdapter) || masterAdapter == nullptr) {
        return;
    }
    // If dags is 1, we have to recreate the adapter.
    if (dags.length() == 1 || !masterAdapter->IsInstanced()) {
        RecreateAdapterOnIdle(id, masterDag.node());
    }
    else {
        // If dags is more than one, trigger rebuilding callbacks next call and
        // mark dirty.
        RebuildAdapterOnIdle(id, MayaHydraSceneIndex::RebuildFlagCallbacks);
        masterAdapter->MarkDirty(
            HdChangeTracker::DirtyInstancer | HdChangeTracker::DirtyInstanceIndex
            | HdChangeTracker::DirtyPrimvar);
    }
}

void MayaHydraSceneIndex::AddArnoldLight(const MDagPath& dag)
{
    _arnoldLightPaths.push_back(dag);
}

void MayaHydraSceneIndex::RemoveArnoldLight(const MDagPath& dag)
{
    const auto it = std::find(_arnoldLightPaths.begin(), _arnoldLightPaths.end(), dag);
    if (it != _arnoldLightPaths.end()) {
        _arnoldLightPaths.erase(it);
    }
}

void MayaHydraSceneIndex::MaterialTagChanged(const SdfPath& id)
{
    if (std::find(_materialTagsChanged.begin(), _materialTagsChanged.end(), id)
        == _materialTagsChanged.end()) {
        _materialTagsChanged.push_back(id);
    }
}

VtValue MayaHydraSceneIndex::GetShadingStyle(SdfPath const& id)
{
    if (auto&& ri = TfMapLookupPtr(_renderItemsAdapters, id)) {
        auto primitive = (*ri)->GetPrimitive();
        if (MHWRender::MGeometry::Primitive::kLines == primitive
            || MHWRender::MGeometry::Primitive::kLineStrip == primitive) {
            return VtValue(
                _tokens
                ->constantLighting); // Use fallbackMaterial + constantLighting + displayColor
        }
    }
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
