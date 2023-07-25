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

#include <mayaHydraLib/sceneIndex/mayaHydraDataSourceRenderItem.h>

#include <pxr/imaging/hd/meshSchema.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include "pxr/imaging/hd/dirtyBitsTranslator.h"


PXR_NAMESPACE_OPEN_SCOPE

namespace {

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

    static const SdfPath lightedObjectsPath = SdfPath(std::string("Lighted"));

    template<class T> SdfPath toSdfPath(const T& src);
    template<> inline SdfPath toSdfPath<MDagPath>(const MDagPath& dag) {
        return MayaHydra::DagPathToSdfPath(dag, false, false);
    }
    template<> inline SdfPath toSdfPath<MRenderItem>(const MRenderItem& ri) {
        return MayaHydra::RenderItemToSdfPath(ri, false);
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
        MayaHydra::SanitizeNameForSdfPath(dependNodeNameString);
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
}

SdfPath MayaHydraSceneIndex::_fallbackMaterial;

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
        _fallbackMaterial = SdfPath::EmptyPath(); // Empty path for hydra fallback material
    });
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
        //if (filterMesh(ri)) {
        //    continue;
        //}

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
        //TODO: Setup Material
        //MObject shadingEngineNode;
        //if (!_GetRenderItemMaterial(ri, material, shadingEngineNode)) {
        //    if (material != kInvalidMaterial) {
        //        _CreateMaterial(material, shadingEngineNode);
        //    }
        //}

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

void MayaHydraSceneIndex::Populate()
{
    // TODO: Uncomment this for Material, Light
    //MayaHydraAdapterRegistry::LoadAllPlugin();
    //auto& renderIndex = GetRenderIndex();
    //MStatus status;
    //MItDag dagIt(MItDag::kDepthFirst);
    //dagIt.traverseUnderWorld(true);
    //if (useMeshAdapter()) {
    //    for (; !dagIt.isDone(); dagIt.next()) {
    //        MDagPath path;
    //        dagIt.getPath(path);
    //        InsertDag(path);
    //    }
    //}
    //else {
    //    for (; !dagIt.isDone(); dagIt.next()) {
    //        MObject node = dagIt.currentItem(&status);
    //        if (status != MS::kSuccess)
    //            continue;
    //        OnDagNodeAdded(node);
    //    }
    //}

    //auto id = MDGMessage::addNodeAddedCallback(_onDagNodeAdded, "dagNode", this, &status);
    //if (status) {
    //    _callbacks.push_back(id);
    //}
    //id = MDGMessage::addNodeRemovedCallback(_onDagNodeRemoved, "dagNode", this, &status);
    //if (status) {
    //    _callbacks.push_back(id);
    //}
    //id = MDGMessage::addConnectionCallback(_connectionChanged, this, &status);
    //if (status) {
    //    _callbacks.push_back(id);
    //}

    //// Adding materials sprim to the render index.
    //if (renderIndex.IsSprimTypeSupported(HdPrimTypeTokens->material)) {
    //    renderIndex.InsertSprim(HdPrimTypeTokens->material, this, _mayaDefaultMaterialPath);
    //}
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

void MayaHydraSceneIndex::PreFrame(const MHWRender::MDrawContext& drawContext)
{
     // TODO: Material, Light
}

void MayaHydraSceneIndex::PostFrame()
{
}

void MayaHydraSceneIndex::InsertRprim(
    MayaHydraAdapter* adapter,
    const TfToken& typeId,
    const SdfPath& id,
    const SdfPath& instancerId)
{
    if (MayaHydraRenderItemAdapter* riAdapter = dynamic_cast<MayaHydraRenderItemAdapter*>(adapter)) {
        auto riDataSource = MayaHydraDataSourceRenderItem::New(
            id, typeId, riAdapter);

        // Add the SceneIndex prim to the scene index
        AddPrims({ { id, typeId, riDataSource } });
    }
}

void MayaHydraSceneIndex::RemoveRprim(const SdfPath& id)
{ 
    RemovePrims({ id });
}

void MayaHydraSceneIndex::MarkRprimDirty(const SdfPath& id, HdDirtyBits dirtyBits)
{
    // Dispatch based on prim type.
    HdSceneIndexPrim prim = GetPrim(id);
    HdDataSourceLocatorSet locators;
    HdDirtyBitsTranslator::RprimDirtyBitsToLocatorSet(prim.primType, dirtyBits, &locators);
    if (!locators.IsEmpty()) {
        DirtyPrims({ {id, locators} });
    }
}

void MayaHydraSceneIndex::InsertSprim(
    const TfToken& typeId,
    const SdfPath& id,
    HdDirtyBits    initialBits)
{
    // TODO: Camera, Light
    // Add the SceneIndex prim to the scene index
    //AddPrims({ { id, typeId, riDataSource } });
}

void MayaHydraSceneIndex::RemoveSprim(const TfToken& typeId, const SdfPath& id)
{
    RemovePrims({ id });
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

// Analogous to MayaHydraSceneDelegate::InsertDag
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

    //if (GetShadingEngineNode(ri, shadingEngineNode))
        // Else try to find associated material node if this is a material shader.
        // NOTE: The existing maya material support in hydra expects a shading engine node
    {
        //material = _GetMaterialPath(shadingEngineNode);
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

PXR_NAMESPACE_CLOSE_SCOPE
