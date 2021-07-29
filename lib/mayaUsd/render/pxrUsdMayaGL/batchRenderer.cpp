//
// Copyright 2016 Pixar
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

// GL loading library needs to be included before any other OpenGL headers.
#include <pxr/pxr.h>
#if PXR_VERSION < 2102
#include <pxr/imaging/glf/glew.h>
#else
#include <pxr/imaging/garch/glApi.h>
#endif

#include "batchRenderer.h"

#include <mayaUsd/render/px_vp20/utils.h>
#include <mayaUsd/render/px_vp20/utils_legacy.h>
#include <mayaUsd/render/pxrUsdMayaGL/debugCodes.h>
#include <mayaUsd/render/pxrUsdMayaGL/userData.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec2i.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/singleton.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stl.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/trace/trace.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/glf/contextCaps.h>
#include <pxr/imaging/hd/changeTracker.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/rprimCollection.h>
#include <pxr/imaging/hd/task.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/hdx/selectionTracker.h>
#include <pxr/imaging/hdx/tokens.h>
#include <pxr/imaging/hgi/hgi.h>
#include <pxr/imaging/hgi/tokens.h>
#include <pxr/usd/sdf/path.h>

#include <maya/M3dView.h>
#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MDrawData.h>
#include <maya/MDrawRequest.h>
#include <maya/MEventMessage.h>
#include <maya/MFileIO.h>
#include <maya/MFrameContext.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MProfiler.h>
#include <maya/MSceneMessage.h>
#include <maya/MSelectInfo.h>
#include <maya/MSelectionContext.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypes.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>

#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((BatchRendererRootName, "MayaHdBatchRenderer"))
    ((LegacyViewport, "LegacyViewport"))
    ((Viewport2, "Viewport2"))
    ((MayaEndRenderNotificationName, "UsdMayaEndRenderNotification"))
);
// clang-format on

TF_INSTANTIATE_SINGLETON(UsdMayaGLBatchRenderer);

const int UsdMayaGLBatchRenderer::ProfilerCategory = MProfiler::addCategory(
#if MAYA_API_VERSION >= 20190000
    "UsdMayaGLBatchRenderer",
    "UsdMayaGLBatchRenderer"
#else
    "UsdMayaGLBatchRenderer"
#endif
);

/* static */
void UsdMayaGLBatchRenderer::Init()
{
#if PXR_VERSION < 2102
    GlfGlewInit();
#endif
    GlfContextCaps::InitInstance();

    GetInstance();
}

/* static */
UsdMayaGLBatchRenderer& UsdMayaGLBatchRenderer::GetInstance()
{
    return TfSingleton<UsdMayaGLBatchRenderer>::GetInstance();
}

HdRenderIndex* UsdMayaGLBatchRenderer::GetRenderIndex() const { return _renderIndex.get(); }

SdfPath UsdMayaGLBatchRenderer::GetDelegatePrefix(const bool isViewport2) const
{
    if (isViewport2) {
        return _viewport2Prefix;
    }

    return _legacyViewportPrefix;
}

bool UsdMayaGLBatchRenderer::AddShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorE_L3, "Batch Renderer Adding Shape Adapter");

    if (!TF_VERIFY(shapeAdapter, "Cannot add invalid shape adapter")) {
        return false;
    }

    const bool isViewport2 = shapeAdapter->IsViewport2();

    // Add the shape adapter to the correct bucket based on its renderParams.
    _ShapeAdapterBucketsMap& bucketsMap
        = isViewport2 ? _shapeAdapterBuckets : _legacyShapeAdapterBuckets;

    const PxrMayaHdRenderParams& renderParams = shapeAdapter->GetRenderParams();
    const size_t                 renderParamsHash = renderParams.Hash();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
        .Msg(
            "Adding shape adapter: %p, isViewport2: %s, renderParamsHash: %zu\n",
            shapeAdapter,
            isViewport2 ? "true" : "false",
            renderParamsHash);

    auto iter = bucketsMap.find(renderParamsHash);
    if (iter == bucketsMap.end()) {
        // If we had no bucket for this particular render param combination
        // then we need to create a new one, but first we make sure that the
        // shape adapter isn't in any other bucket.
        RemoveShapeAdapter(shapeAdapter);

        bucketsMap[renderParamsHash]
            = _ShapeAdapterBucket(renderParams, _ShapeAdapterSet({ shapeAdapter }));

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg("    Added to newly created bucket\n");
    } else {
        // Check whether this shape adapter is already in this bucket.
        _ShapeAdapterSet& shapeAdapters = iter->second.second;
        auto              setIter = shapeAdapters.find(shapeAdapter);
        if (setIter != shapeAdapters.end()) {
            // If it's already in this bucket, then we're done, and we didn't
            // have to add it.
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                .Msg("    Not adding, already in correct bucket\n");

            return false;
        }

        // Otherwise, remove it from any other bucket it may be in before
        // adding it to this one.
        RemoveShapeAdapter(shapeAdapter);

        shapeAdapters.insert(shapeAdapter);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING).Msg("    Added to existing bucket\n");
    }

    // Debug dumping of current bucket state.
    if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)) {
        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
            .Msg("    _shapeAdapterBuckets (Viewport 2.0) contents:\n");
        for (const auto& iter : _shapeAdapterBuckets) {
            const size_t            bucketHash = iter.first;
            const _ShapeAdapterSet& shapeAdapters = iter.second.second;

            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                .Msg(
                    "        renderParamsHash: %zu, bucket size: %zu\n",
                    bucketHash,
                    shapeAdapters.size());

            for (const auto shapeAdapter : shapeAdapters) {
                TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                    .Msg("            shape adapter: %p\n", shapeAdapter);
            }
        }

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
            .Msg("    _legacyShapeAdapterBuckets (Legacy viewport) contents:\n");
        for (auto& iter : _legacyShapeAdapterBuckets) {
            const size_t            bucketHash = iter.first;
            const _ShapeAdapterSet& shapeAdapters = iter.second.second;

            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                .Msg(
                    "        renderParamsHash: %zu, bucket size: %zu\n",
                    bucketHash,
                    shapeAdapters.size());

            for (const auto shapeAdapter : shapeAdapters) {
                TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                    .Msg("            shape adapter: %p\n", shapeAdapter);
            }
        }
    }

    // Add the shape adapter to the secondary object handle map.
    _ShapeAdapterHandleMap& handleMap
        = isViewport2 ? _shapeAdapterHandleMap : _legacyShapeAdapterHandleMap;
    handleMap[MObjectHandle(shapeAdapter->GetDagPath().node())] = shapeAdapter;

    return true;
}

bool UsdMayaGLBatchRenderer::RemoveShapeAdapter(PxrMayaHdShapeAdapter* shapeAdapter)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorE_L3, "Batch Renderer Removing Shape Adapter");

    if (!TF_VERIFY(shapeAdapter, "Cannot remove invalid shape adapter")) {
        return false;
    }

    const bool isViewport2 = shapeAdapter->IsViewport2();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
        .Msg(
            "Removing shape adapter: %p, isViewport2: %s\n",
            shapeAdapter,
            isViewport2 ? "true" : "false");

    // Remove shape adapter from its bucket in the bucket map.
    _ShapeAdapterBucketsMap& bucketsMap
        = isViewport2 ? _shapeAdapterBuckets : _legacyShapeAdapterBuckets;

    size_t numErased = 0u;

    std::vector<size_t> emptyBucketHashes;

    for (auto& iter : bucketsMap) {
        const size_t      renderParamsHash = iter.first;
        _ShapeAdapterSet& shapeAdapters = iter.second.second;

        const size_t numBefore = numErased;
        numErased += shapeAdapters.erase(shapeAdapter);

        if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING) && numErased > numBefore) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                .Msg("    Removed from bucket with render params hash: %zu\n", renderParamsHash);
        }

        if (shapeAdapters.empty()) {
            // This bucket is now empty, so we tag it for removal below.
            emptyBucketHashes.push_back(renderParamsHash);
        }
    }

    // Remove any empty buckets.
    for (const size_t renderParamsHash : emptyBucketHashes) {
        const size_t numErasedBuckets = bucketsMap.erase(renderParamsHash);

        if (TfDebug::IsEnabled(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING) && numErasedBuckets > 0u) {
            TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_BUCKETING)
                .Msg("    Removed empty bucket with render params hash: %zu\n", renderParamsHash);
        }
    }

    // Remove shape adapter from the secondary DAG path map.
    _ShapeAdapterHandleMap& handleMap
        = isViewport2 ? _shapeAdapterHandleMap : _legacyShapeAdapterHandleMap;
    handleMap.erase(MObjectHandle(shapeAdapter->GetDagPath().node()));

    return (numErased > 0u);
}

/* static */
void UsdMayaGLBatchRenderer::Reset()
{
    if (UsdMayaGLBatchRenderer::CurrentlyExists()) {
        TF_STATUS("Resetting USD Batch Renderer");
        UsdMayaGLBatchRenderer::DeleteInstance();
    }

    UsdMayaGLBatchRenderer::GetInstance();
}

bool UsdMayaGLBatchRenderer::PopulateCustomPrimFilter(
    const MDagPath&      dagPath,
    PxrMayaHdPrimFilter& primFilter)
{
    // We're drawing "out-of-band", so it doesn't matter if we grab the VP2
    // or the Legacy shape adapter. Prefer VP2, but fall back to Legacy if
    // we can't find the VP2 adapter.
    MObjectHandle objHandle(dagPath.node());
    auto          iter = _shapeAdapterHandleMap.find(objHandle);
    if (iter == _shapeAdapterHandleMap.end()) {
        iter = _legacyShapeAdapterHandleMap.find(objHandle);
        if (iter == _legacyShapeAdapterHandleMap.end()) {
            return false;
        }
    }

    HdRprimCollection& collection = primFilter.collection;

    // Doesn't really hurt to always add, and ensures that the collection is
    // tracked properly.
    HdChangeTracker& changeTracker = _renderIndex->GetChangeTracker();
    changeTracker.AddCollection(collection.GetName());

    // Only update the collection and mark it dirty if the root paths have
    // actually changed. This greatly affects performance.
    PxrMayaHdShapeAdapter* adapter = iter->second;
    const HdReprSelector   repr = collection.GetReprSelector();
    const SdfPathVector&   roots = adapter->GetRprimCollection(repr).GetRootPaths();
    if (collection.GetRootPaths() != roots) {
        collection.SetRootPaths(roots);
        changeTracker.MarkCollectionDirty(collection.GetName());
    }

    primFilter.renderTags = adapter->GetRenderTags();

    return true;
}

// Since we're using a static singleton UsdMayaGLBatchRenderer object, we need
// to make sure that we reset its state when switching to a new Maya scene or
// when opening a different scene.
void UsdMayaGLBatchRenderer::_OnMayaSceneReset(const UsdMayaSceneResetNotice& notice)
{
    UsdMayaGLBatchRenderer::Reset();
}

// For Viewport 2.0, we listen for a notification from Maya's rendering
// pipeline that all render passes have completed and then we do some cleanup.
/* static */
void UsdMayaGLBatchRenderer::_OnMayaEndRenderCallback(
    MHWRender::MDrawContext& context,
    void*                    clientData)
{
    if (UsdMayaGLBatchRenderer::CurrentlyExists()) {
        UsdMayaGLBatchRenderer::GetInstance()._MayaRenderDidEnd(&context);
    }
}

/* static */
void UsdMayaGLBatchRenderer::_OnSoftSelectOptionsChangedCallback(void* clientData)
{
    auto batchRenderer = static_cast<UsdMayaGLBatchRenderer*>(clientData);
    int  commandResult;
    // -sse == -softSelectEnabled
    MGlobal::executeCommand("softSelect -q -sse", commandResult);
    if (!commandResult) {
        batchRenderer->_objectSoftSelectEnabled = false;
        return;
    }
    // -ssf == -softSelectFalloff
    MGlobal::executeCommand("softSelect -q -ssf", commandResult);
    // fallbackMode 3 == object mode
    batchRenderer->_objectSoftSelectEnabled = (commandResult == 3);
}

UsdMayaGLBatchRenderer::UsdMayaGLBatchRenderer()
    : _isSelectionPending(false)
    , _objectSoftSelectEnabled(false)
    , _softSelectOptionsCallbackId(0)
    , _selectResultsKey(GfMatrix4d(0.0), GfMatrix4d(0.0), false)
#if PXR_VERSION > 2005
    , _hgi(Hgi::CreatePlatformDefaultHgi())
#else
    , _hgi(Hgi::GetPlatformDefaultHgi())
#endif
    , _hgiDriver { HgiTokens->renderDriver, VtValue(_hgi.get()) }
    , _selectionResolution(256)
    , _enableDepthSelection(false)
{
    _rootId = SdfPath::AbsoluteRootPath().AppendChild(_tokens->BatchRendererRootName);
    _legacyViewportPrefix = _rootId.AppendChild(_tokens->LegacyViewport);
    _viewport2Prefix = _rootId.AppendChild(_tokens->Viewport2);

    _renderIndex.reset(HdRenderIndex::New(&_renderDelegate, { &_hgiDriver }));
    if (!TF_VERIFY(_renderIndex)) {
        return;
    }

    _taskDelegate.reset(new PxrMayaHdSceneDelegate(_renderIndex.get(), _rootId));

    _legacyViewportRprimCollection.SetName(TfToken(TfStringPrintf(
        "%s_%s", _tokens->BatchRendererRootName.GetText(), _tokens->LegacyViewport.GetText())));
    _legacyViewportRprimCollection.SetReprSelector(HdReprSelector(HdReprTokens->refined));
    _legacyViewportRprimCollection.SetRootPath(_legacyViewportPrefix);
    _renderIndex->GetChangeTracker().AddCollection(_legacyViewportRprimCollection.GetName());

    _viewport2RprimCollection.SetName(TfToken(TfStringPrintf(
        "%s_%s", _tokens->BatchRendererRootName.GetText(), _tokens->Viewport2.GetText())));
    _viewport2RprimCollection.SetReprSelector(HdReprSelector(HdReprTokens->refined));
    _viewport2RprimCollection.SetRootPath(_viewport2Prefix);
    _renderIndex->GetChangeTracker().AddCollection(_viewport2RprimCollection.GetName());

    _selectionTracker.reset(new HdxSelectionTracker());

    TfWeakPtr<UsdMayaGLBatchRenderer> me(this);
    TfNotice::Register(me, &UsdMayaGLBatchRenderer::_OnMayaSceneReset);

    MHWRender::MRenderer* renderer = MHWRender::MRenderer::theRenderer();
    if (!renderer) {
        TF_RUNTIME_ERROR("Viewport 2.0 renderer not initialized.");
    } else {
        // Note that we do not ever remove this notification handler. Maya
        // ensures that only one handler will be registered for a given name
        // and semantic location.
        renderer->addNotification(
            UsdMayaGLBatchRenderer::_OnMayaEndRenderCallback,
            _tokens->MayaEndRenderNotificationName.GetText(),
            MHWRender::MPassContext::kEndRenderSemantic,
            nullptr);
    }

    // We call _OnSoftSelectOptionsChangedCallback manually once, to initalize
    // _objectSoftSelectEnabled; because of this, it's setup is slightly
    // different - since we're calling from within the constructor, we don't
    // use CurrentlyExists()/GetInstance(), but instead pass this as clientData;
    // this also means we should clean up the callback in the destructor.
    _OnSoftSelectOptionsChangedCallback(this);
    _softSelectOptionsCallbackId = MEventMessage::addEventCallback(
        "softSelectOptionsChanged", _OnSoftSelectOptionsChangedCallback, this);
}

/* virtual */
UsdMayaGLBatchRenderer::~UsdMayaGLBatchRenderer()
{
    _selectionTracker.reset();
    _taskDelegate.reset();

    // We remove the softSelectOptionsChanged callback because it's passed
    // a this pointer, while others aren't.  We do that, instead of just
    // using CurrentlyExists()/GetInstance() because we call it within the
    // constructor
    MMessage::removeCallback(_softSelectOptionsCallbackId);
}

const UsdMayaGLSoftSelectHelper& UsdMayaGLBatchRenderer::GetSoftSelectHelper()
{
    _softSelectHelper.Populate();
    return _softSelectHelper;
}

static void _GetWorldToViewMatrix(M3dView& view, GfMatrix4d* worldToViewMatrix)
{
    // Legacy viewport implementation.

    if (!worldToViewMatrix) {
        return;
    }

    // Note that we use GfMatrix4d's GetInverse() method to get the
    // world-to-view matrix from the camera matrix and NOT MMatrix's
    // inverse(). The latter was introducing very small bits of floating
    // point error that would sometimes result in the positions of lights
    // being computed downstream as having w coordinate values that were
    // very close to but not exactly 1.0 or 0.0. When drawn, the light
    // would then flip between being a directional light (w = 0.0) and a
    // non-directional light (w = 1.0).
    MDagPath cameraDagPath;
    view.getCamera(cameraDagPath);
    const GfMatrix4d cameraMatrix(cameraDagPath.inclusiveMatrix().matrix);
    *worldToViewMatrix = cameraMatrix.GetInverse();
}

static void _GetViewport(M3dView& view, GfVec4d* viewport)
{
    // Legacy viewport implementation.

    if (!viewport) {
        return;
    }

    unsigned int viewX, viewY, viewWidth, viewHeight;
    view.viewport(viewX, viewY, viewWidth, viewHeight);
    *viewport = GfVec4d(viewX, viewY, viewWidth, viewHeight);
}

static void
_GetWorldToViewMatrix(const MHWRender::MDrawContext& context, GfMatrix4d* worldToViewMatrix)
{
    // Viewport 2.0 implementation.

    if (!worldToViewMatrix) {
        return;
    }

    MStatus       status;
    const MMatrix viewMat = context.getMatrix(MHWRender::MFrameContext::kViewMtx, &status);
    *worldToViewMatrix = GfMatrix4d(viewMat.matrix);
}

static void _GetViewport(const MHWRender::MDrawContext& context, GfVec4d* viewport)
{
    // Viewport 2.0 implementation.

    if (!viewport) {
        return;
    }

    int viewX, viewY, viewWidth, viewHeight;
    context.getViewportDimensions(viewX, viewY, viewWidth, viewHeight);
    *viewport = GfVec4d(viewX, viewY, viewWidth, viewHeight);
}

void UsdMayaGLBatchRenderer::Draw(const MDrawRequest& request, M3dView& view)
{
    // Legacy viewport implementation.

    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorC_L2, "Batch Renderer Draw() (Legacy Viewport)");

    MDrawData drawData = request.drawData();

    const PxrMayaHdUserData* hdUserData
        = static_cast<const PxrMayaHdUserData*>(drawData.geometry());
    if (!hdUserData) {
        return;
    }

    GfMatrix4d worldToViewMatrix;
    _GetWorldToViewMatrix(view, &worldToViewMatrix);

    MMatrix projectionMat;
    view.projectionMatrix(projectionMat);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    GfVec4d viewport;
    _GetViewport(view, &viewport);

    _RenderBatches(
        /* vp2Context */ nullptr, &view, worldToViewMatrix, projectionMatrix, viewport);

    // Clean up the user data.
    delete hdUserData;
}

void UsdMayaGLBatchRenderer::Draw(const MHWRender::MDrawContext& context, const MUserData* userData)
{
    // Viewport 2.0 implementation.

    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorC_L2, "Batch Renderer Draw() (Viewport 2.0)");

    const PxrMayaHdUserData* hdUserData = dynamic_cast<const PxrMayaHdUserData*>(userData);
    if (!hdUserData) {
        return;
    }

    const MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer || !theRenderer->drawAPIIsOpenGL()) {
        return;
    }

    // Check whether this draw call is for a selection pass. If it is, we do
    // *not* actually perform any drawing, but instead just mark a selection as
    // pending so we know to re-compute selection when the next pick attempt is
    // made.
    // Note that Draw() calls for contexts with the "selectionPass" semantic
    // are only made from draw overrides that do *not* implement user selection
    // (i.e. those that do not override, or return false from,
    // wantUserSelection()). The draw override for pxrHdImagingShape will
    // likely be the only one of these where that is the case.
    const MHWRender::MPassContext& passContext = context.getPassContext();
    const MStringArray&            passSemantics = passContext.passSemantics();

    for (unsigned int i = 0u; i < passSemantics.length(); ++i) {
        if (passSemantics[i] == MHWRender::MPassContext::kSelectionPassSemantic) {
            _UpdateIsSelectionPending(true);
            return;
        }
    }

    GfMatrix4d worldToViewMatrix;
    _GetWorldToViewMatrix(context, &worldToViewMatrix);

    MStatus status;

    const MMatrix projectionMat
        = context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    GfVec4d viewport;
    _GetViewport(context, &viewport);

    M3dView    view;
    const bool hasView = px_vp20Utils::GetViewFromDrawContext(context, view);

    _RenderBatches(
        &context, hasView ? &view : nullptr, worldToViewMatrix, projectionMatrix, viewport);
}

void UsdMayaGLBatchRenderer::DrawBoundingBox(const MDrawRequest& request, M3dView& view)
{
    // Legacy viewport implementation.

    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory,
        MProfiler::kColorC_L2,
        "Batch Renderer DrawBoundingBox() (Legacy Viewport)");

    MDrawData drawData = request.drawData();

    const PxrMayaHdUserData* hdUserData
        = static_cast<const PxrMayaHdUserData*>(drawData.geometry());
    if (!hdUserData || !hdUserData->boundingBox) {
        return;
    }

    MMatrix modelViewMat;
    view.modelViewMatrix(modelViewMat);

    MMatrix projectionMat;
    view.projectionMatrix(projectionMat);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    // For the legacy viewport, apply a framebuffer gamma correction when
    // drawing bounding boxes, just like we do when drawing geometry via Hydra.
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);

    px_vp20Utils::RenderBoundingBox(
        *(hdUserData->boundingBox), *(hdUserData->wireframeColor), modelViewMat, projectionMat);

    glDisable(GL_FRAMEBUFFER_SRGB_EXT);

    // Clean up the user data.
    delete hdUserData;
}

void UsdMayaGLBatchRenderer::DrawBoundingBox(
    const MHWRender::MDrawContext& context,
    const MUserData*               userData)
{
    // Viewport 2.0 implementation.

    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorC_L2, "Batch Renderer DrawBoundingBox() (Viewport 2.0)");

    const PxrMayaHdUserData* hdUserData = dynamic_cast<const PxrMayaHdUserData*>(userData);
    if (!hdUserData || !hdUserData->boundingBox) {
        return;
    }

    const MHWRender::MRenderer* theRenderer = MHWRender::MRenderer::theRenderer();
    if (!theRenderer || !theRenderer->drawAPIIsOpenGL()) {
        return;
    }

    MStatus status;

    const MMatrix worldViewMat
        = context.getMatrix(MHWRender::MFrameContext::kWorldViewMtx, &status);

    const MMatrix projectionMat
        = context.getMatrix(MHWRender::MFrameContext::kProjectionMtx, &status);
    const GfMatrix4d projectionMatrix(projectionMat.matrix);

    px_vp20Utils::RenderBoundingBox(
        *(hdUserData->boundingBox), *(hdUserData->wireframeColor), worldViewMat, projectionMat);
}

GfVec2i UsdMayaGLBatchRenderer::GetSelectionResolution() const { return _selectionResolution; }

void UsdMayaGLBatchRenderer::SetSelectionResolution(const GfVec2i& widthHeight)
{
    _selectionResolution = widthHeight;
}

bool UsdMayaGLBatchRenderer::IsDepthSelectionEnabled() const { return _enableDepthSelection; }

void UsdMayaGLBatchRenderer::SetDepthSelectionEnabled(const bool enabled)
{
    _enableDepthSelection = enabled;
}

const HdxPickHitVector* UsdMayaGLBatchRenderer::TestIntersection(
    const PxrMayaHdShapeAdapter* shapeAdapter,
    MSelectInfo&                 selectInfo)
{
    // Legacy viewport implementation.

    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory,
        MProfiler::kColorE_L3,
        "Batch Renderer Testing Intersection (Legacy Viewport)");

    // Guard against the user clicking in the viewer before the renderer is
    // setup, or with no shape adapters registered.
    if (!_renderIndex || _legacyShapeAdapterBuckets.empty()) {
        _selectResults.clear();
        return nullptr;
    }

    M3dView view = selectInfo.view();

    if (_UpdateIsSelectionPending(false)) {
        if (TfDebug::IsEnabled(PXRUSDMAYAGL_BATCHED_SELECTION)) {
            TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
                .Msg("Computing batched selection for legacy viewport\n");
        }

        GfMatrix4d viewMatrix;
        GfMatrix4d projectionMatrix;
        px_LegacyViewportUtils::GetSelectionMatrices(selectInfo, viewMatrix, projectionMatrix);

        _ComputeSelection(
            _legacyShapeAdapterBuckets,
            &view,
            viewMatrix,
            projectionMatrix,
            selectInfo.singleSelection());
    }

    const HdxPickHitVector* const hitSet
        = TfMapLookupPtr(_selectResults, shapeAdapter->GetDelegateID());
    if (!hitSet || hitSet->empty()) {
        if (_selectResults.empty()) {
            // If nothing was selected previously AND nothing is selected now,
            // Maya does not refresh the viewport. This would be fine, except
            // that we need to make sure we're ready to respond to another
            // selection. Maya may be calling select() on many shapes in
            // series, so we cannot mark a selection pending here or we will
            // end up re-computing the selection on every call. Instead we
            // simply schedule a refresh of the viewport, at the end of which
            // the end render callback will be invoked and we'll mark a
            // selection pending then.
            view.scheduleRefresh();
        }

        return nullptr;
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION).Msg("    FOUND %zu HIT(s)\n", hitSet->size());
    if (TfDebug::IsEnabled(PXRUSDMAYAGL_BATCHED_SELECTION)) {
        for (const HdxPickHit& hit : *hitSet) {
            TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
                .Msg(
                    "        HIT:\n"
                    "            delegateId      : %s\n"
                    "            objectId        : %s\n"
                    "            normalizedDepth : %f\n",
                    hit.delegateId.GetText(),
                    hit.objectId.GetText(),
                    hit.normalizedDepth);
        }
    }

    return hitSet;
}

const HdxPickHitVector* UsdMayaGLBatchRenderer::TestIntersection(
    const PxrMayaHdShapeAdapter*     shapeAdapter,
    const MHWRender::MSelectionInfo& selectionInfo,
    const MHWRender::MDrawContext&   context)
{
    // Viewport 2.0 implementation.

    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory,
        MProfiler::kColorE_L3,
        "Batch Renderer Testing Intersection (Viewport 2.0)");

    // Guard against the user clicking in the viewer before the renderer is
    // setup, or with no shape adapters registered.
    if (!_renderIndex || _shapeAdapterBuckets.empty()) {
        _selectResults.clear();
        return nullptr;
    }

    GfMatrix4d viewMatrix;
    GfMatrix4d projectionMatrix;
    if (!px_vp20Utils::GetSelectionMatrices(selectionInfo, context, viewMatrix, projectionMatrix)) {
        return nullptr;
    }

    const bool wasSelectionPending = _UpdateIsSelectionPending(false);

    const bool singleSelection = selectionInfo.singleSelection();

    // Typically, we rely on the _isSelectionPending state to determine if we can
    // re-use the previously computed select results.  However, there are cases
    // (e.g. Pre-selection hilighting) where we call userSelect without a new
    // draw call (which typically resets the _isSelectionPending).
    //
    // In these cases, we look at the projectionMatrix for the selection as well
    // to see if the selection needs to be re-computed.
    const _SelectResultsKey key = std::make_tuple(viewMatrix, projectionMatrix, singleSelection);
    const bool              newSelKey = key != _selectResultsKey;

    const bool needToRecomputeSelection = wasSelectionPending || newSelKey;
    if (needToRecomputeSelection) {
        if (TfDebug::IsEnabled(PXRUSDMAYAGL_BATCHED_SELECTION)) {
            TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
                .Msg("Computing batched selection for Viewport 2.0\n");

            const MUint64                  frameStamp = context.getFrameStamp();
            const MHWRender::MPassContext& passContext = context.getPassContext();
            const MString&                 passId = passContext.passIdentifier();
            const MStringArray&            passSemantics = passContext.passSemantics();

            TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
                .Msg(
                    "    frameStamp: %s, passIdentifier: %s, passSemantics: %s\n",
                    TfStringify<MUint64>(frameStamp).c_str(),
                    passId.asChar(),
                    TfStringify<MStringArray>(passSemantics).c_str());
        }

        M3dView    view;
        const bool hasView = px_vp20Utils::GetViewFromDrawContext(context, view);

        _ComputeSelection(
            _shapeAdapterBuckets,
            hasView ? &view : nullptr,
            viewMatrix,
            projectionMatrix,
            singleSelection);
        _selectResultsKey = key;
    }

    const HdxPickHitVector* const hitSet
        = TfMapLookupPtr(_selectResults, shapeAdapter->GetDelegateID());
    if (!hitSet || hitSet->empty()) {
        return nullptr;
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION).Msg("    FOUND %zu HIT(s)\n", hitSet->size());
    if (TfDebug::IsEnabled(PXRUSDMAYAGL_BATCHED_SELECTION)) {
        for (const HdxPickHit& hit : *hitSet) {
            TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
                .Msg(
                    "        HIT:\n"
                    "            delegateId      : %s\n"
                    "            objectId        : %s\n"
                    "            normalizedDepth : %f\n",
                    hit.delegateId.GetText(),
                    hit.objectId.GetText(),
                    hit.normalizedDepth);
        }
    }

    return hitSet;
}

bool UsdMayaGLBatchRenderer::TestIntersectionCustomPrimFilter(
    const PxrMayaHdPrimFilter& primFilter,
    const GfMatrix4d&          viewMatrix,
    const GfMatrix4d&          projectionMatrix,
    HdxPickHitVector*          outResult)
{
    // Custom collection implementation.
    // Differs from viewport implementations in that it doesn't rely on
    // _ComputeSelection being called first.

    GLUniformBufferBindingsSaver bindingsSaver;

    return _TestIntersection(
        primFilter.collection,
        primFilter.renderTags,
        viewMatrix,
        projectionMatrix,
        true,
        outResult);
}

/* static */
const HdxPickHit* UsdMayaGLBatchRenderer::GetNearestHit(const HdxPickHitVector* const hitSet)
{
    if (!hitSet || hitSet->empty()) {
        return nullptr;
    }

    const HdxPickHit* minHit = &(*hitSet->begin());

    for (const auto& hit : *hitSet) {
        if (hit < *minHit) {
            minHit = &hit;
        }
    }

    return minHit;
}

PxrMayaHdPrimFilterVector UsdMayaGLBatchRenderer::_GetIntersectionPrimFilters(
    _ShapeAdapterBucketsMap& bucketsMap,
    const M3dView*           view,
    const bool               useDepthSelection) const
{
    PxrMayaHdPrimFilterVector primFilters;

    if (bucketsMap.empty()) {
        return primFilters;
    }

    // Assume the shape adapters are for Viewport 2.0 until we inspect the
    // first one.
    bool isViewport2 = true;

    for (auto& iter : bucketsMap) {
        _ShapeAdapterSet& shapeAdapters = iter.second.second;

        for (PxrMayaHdShapeAdapter* shapeAdapter : shapeAdapters) {
            shapeAdapter->UpdateVisibility(view);

            isViewport2 = shapeAdapter->IsViewport2();

            if (!useDepthSelection) {
                // If we don't care about selecting in depth, only update
                // visibility for the shape adapters. We'll use the full
                // viewport renderer collection for selection instead of the
                // individual shape adapter collections.
                continue;
            }

            // XXX: The full viewport-based collections use the "refined" repr,
            // so we use the same repr here if we're doing adapter-by-adapter
            // depth selection. Ideally though, this would be whatever repr was
            // most recently drawn for the viewport in which the selection is
            // taking place.
            const HdReprSelector     repr(HdReprTokens->refined);
            const HdRprimCollection& rprimCollection = shapeAdapter->GetRprimCollection(repr);

            const TfTokenVector& renderTags = shapeAdapter->GetRenderTags();

            primFilters.push_back(
                PxrMayaHdPrimFilter { shapeAdapter, rprimCollection, renderTags });
        }
    }

    if (!useDepthSelection) {
        const HdRprimCollection& collection
            = isViewport2 ? _viewport2RprimCollection : _legacyViewportRprimCollection;

        primFilters.push_back(PxrMayaHdPrimFilter {
            nullptr,
            collection,
            TfTokenVector { HdRenderTagTokens->geometry, HdRenderTagTokens->proxy } });
    }

    return primFilters;
}

bool UsdMayaGLBatchRenderer::_TestIntersection(
    const HdRprimCollection& rprimCollection,
    const TfTokenVector&     renderTags,
    const GfMatrix4d&        viewMatrix,
    const GfMatrix4d&        projectionMatrix,
    const bool               singleSelection,
    HdxPickHitVector*        result)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorE_L3, "Batch Renderer Testing Intersection");

    if (!result) {
        return false;
    }

    glPushAttrib(
        GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        | GL_STENCIL_BUFFER_BIT | GL_TEXTURE_BIT | GL_POLYGON_BIT);

    // hydra orients all geometry during topological processing so that
    // front faces have ccw winding. We disable culling because culling
    // is handled by fragment shader discard.
    glFrontFace(GL_CCW); // < State is pushed via GL_POLYGON_BIT
    glDisable(GL_CULL_FACE);

    // note: to get benefit of alpha-to-coverage, the target framebuffer
    // has to be a MSAA buffer.
    glDisable(GL_BLEND);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    HdTaskSharedPtrVector tasks;
    tasks = _taskDelegate->GetPickingTasks(renderTags);

    HdxPickTaskContextParams pickParams;
    pickParams.resolution = _selectionResolution;
    pickParams.resolveMode
        = singleSelection ? HdxPickTokens->resolveNearestToCenter : HdxPickTokens->resolveUnique;
    pickParams.viewMatrix = viewMatrix;
    pickParams.projectionMatrix = projectionMatrix;
    pickParams.collection = rprimCollection;
    pickParams.outHits = result;

    VtValue vtPickParams(pickParams);
    _hdEngine.SetTaskContextData(HdxPickTokens->pickParams, vtPickParams);
    _hdEngine.Execute(_renderIndex.get(), &tasks);

    glPopAttrib();

    return (result->size() > 0);
}

void UsdMayaGLBatchRenderer::_ComputeSelection(
    _ShapeAdapterBucketsMap& bucketsMap,
    const M3dView*           view3d,
    const GfMatrix4d&        viewMatrix,
    const GfMatrix4d&        projectionMatrix,
    const bool               singleSelection)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorE_L3, "Batch Renderer Computing Selection");

    // If depth selection has not been turned on, then we can optimize
    // area/marquee selections by handling collections similarly to a single
    // selection, where we test intersections against the single, viewport
    // renderer-based collection.
    const bool useDepthSelection = (!singleSelection && _enableDepthSelection);

    const PxrMayaHdPrimFilterVector primFilters
        = _GetIntersectionPrimFilters(bucketsMap, view3d, useDepthSelection);

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
        .Msg(
            "    ____________ SELECTION STAGE START ______________ "
            "(singleSelection = %s, %zu prim filter(s))\n",
            singleSelection ? "true" : "false",
            primFilters.size());

    _selectResults.clear();

    GLUniformBufferBindingsSaver bindingsSaver;

    for (const PxrMayaHdPrimFilter& primFilter : primFilters) {
        TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
            .Msg(
                "    --- Intersection Testing with collection: %s\n",
                primFilter.collection.GetName().GetText());

        HdxPickHitVector hits;
        if (!_TestIntersection(
                primFilter.collection,
                primFilter.renderTags,
                viewMatrix,
                projectionMatrix,
                singleSelection,
                &hits)) {
            continue;
        }

        for (const HdxPickHit& hit : hits) {
            const auto itIfExists
                = _selectResults.emplace(std::make_pair(hit.delegateId, HdxPickHitVector({ hit })));

            const bool& inserted = itIfExists.second;
            if (!inserted) {
                _selectResults[hit.delegateId].push_back(hit);
            }
        }
    }

    // Populate the Hydra selection from the selection results.
    HdSelectionSharedPtr selection(new HdSelection);

    const HdSelection::HighlightMode selectionMode = HdSelection::HighlightModeSelect;

    for (const auto& delegateHits : _selectResults) {
        const HdxPickHitVector& hitSet = delegateHits.second;

        for (const HdxPickHit& hit : hitSet) {
            TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
                .Msg(
                    "    NEW HIT\n"
                    "        delegateId      : %s\n"
                    "        objectId        : %s\n"
                    "        instanceIndex   : %d\n"
                    "        normalizedDepth : %f\n",
                    hit.delegateId.GetText(),
                    hit.objectId.GetText(),
                    hit.instanceIndex,
                    hit.normalizedDepth);

            if (!hit.instancerId.IsEmpty()) {
                const VtIntArray instanceIndices(1, hit.instanceIndex);
                selection->AddInstance(selectionMode, hit.objectId, instanceIndices);
            } else {
                selection->AddRprim(selectionMode, hit.objectId);
            }
        }
    }

    _selectionTracker->SetSelection(selection);

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_SELECTION)
        .Msg("    ^^^^^^^^^^^^ SELECTION STAGE FINISH ^^^^^^^^^^^^^\n");
}

void UsdMayaGLBatchRenderer::_Render(
    const GfMatrix4d&               worldToViewMatrix,
    const GfMatrix4d&               projectionMatrix,
    const GfVec4d&                  viewport,
    unsigned int                    displayStyle,
    const std::vector<_RenderItem>& items)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorC_L2, "Batch Renderer Rendering Batch");

    _taskDelegate->SetCameraState(worldToViewMatrix, projectionMatrix, viewport);

    // save the current GL states which hydra may reset to default
    glPushAttrib(
        GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT);

    GLUniformBufferBindingsSaver bindingsSaver;

    // hydra orients all geometry during topological processing so that
    // front faces have ccw winding. We disable culling because culling
    // is handled by fragment shader discard.
    glFrontFace(GL_CCW); // < State is pushed via GL_POLYGON_BIT
    glDisable(GL_CULL_FACE);

    // note: to get benefit of alpha-to-coverage, the target framebuffer
    // has to be a MSAA buffer.
    glDisable(GL_BLEND);
    glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // In all cases, we can should enable gamma correction:
    // - in viewport 1.0, we're expected to do it
    // - in viewport 2.0 without color correction, we're expected to do it
    // - in viewport 2.0 with color correction, the render target ignores this
    //   bit meaning we properly are blending linear colors in the render
    //   target.  The color management pipeline is responsible for the final
    //   correction.
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render task setup
    HdTaskSharedPtrVector tasks = _taskDelegate->GetSetupTasks(); // lighting etc

    for (const auto& iter : items) {
        const PxrMayaHdRenderParams& params = iter.first;
        const size_t                 paramsHash = params.Hash();

        const PxrMayaHdPrimFilterVector& primFilters = iter.second;

        TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
            .Msg(
                "    *** renderBucket, parameters hash: %zu, bucket size %zu\n",
                paramsHash,
                primFilters.size());

        HdTaskSharedPtrVector renderTasks
            = _taskDelegate->GetRenderTasks(paramsHash, params, displayStyle, primFilters);
        tasks.insert(tasks.end(), renderTasks.begin(), renderTasks.end());
    }

    VtValue selectionTrackerValue(_selectionTracker);
    _hdEngine.SetTaskContextData(HdxTokens->selectionState, selectionTrackerValue);

    {
        TRACE_SCOPE("Executing Hydra Tasks");

        MProfilingScope hydraProfilingScope(
            ProfilerCategory, MProfiler::kColorC_L3, "Batch Renderer Executing Hydra Tasks");

        _hdEngine.Execute(_renderIndex.get(), &tasks);
    }

    glDisable(GL_FRAMEBUFFER_SRGB_EXT);

    glPopAttrib(); // GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_POLYGON_BIT |
                   // GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT
}

void UsdMayaGLBatchRenderer::_RenderBatches(
    const MHWRender::MDrawContext* vp2Context,
    const M3dView*                 view3d,
    const GfMatrix4d&              worldToViewMatrix,
    const GfMatrix4d&              projectionMatrix,
    const GfVec4d&                 viewport)
{
    TRACE_FUNCTION();

    MProfilingScope profilingScope(
        ProfilerCategory, MProfiler::kColorC_L2, "Batch Renderer Rendering Batches");

    _ShapeAdapterBucketsMap& bucketsMap
        = bool(vp2Context) ? _shapeAdapterBuckets : _legacyShapeAdapterBuckets;

    if (bucketsMap.empty()) {
        return;
    }

    if (TfDebug::IsEnabled(PXRUSDMAYAGL_BATCHED_DRAWING)) {
        TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
            .Msg("Drawing batches for %s\n", bool(vp2Context) ? "Viewport 2.0" : "legacy viewport");

        if (vp2Context) {
            const MUint64                  frameStamp = vp2Context->getFrameStamp();
            const MHWRender::MPassContext& passContext = vp2Context->getPassContext();
            const MString&                 passId = passContext.passIdentifier();
            const MStringArray&            passSemantics = passContext.passSemantics();

            TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
                .Msg(
                    "    frameStamp: %s, passIdentifier: %s, passSemantics: %s\n",
                    TfStringify<MUint64>(frameStamp).c_str(),
                    passId.asChar(),
                    TfStringify<MStringArray>(passSemantics).c_str());
        }
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
        .Msg(
            "    ____________ RENDER STAGE START ______________ (%zu buckets)\n",
            bucketsMap.size());

    // A new display refresh signifies that the cached selection data is no
    // longer valid.
    _selectResults.clear();

    // We've already populated with all the selection info we need.  We Reset
    // and the first call to GetSoftSelectHelper in the next render pass will
    // re-populate it.
    _softSelectHelper.Reset();

    // Assume shaded displayStyle, but we *should* be able to pull it from
    // either the vp2Context for Viewport 2.0 or the M3dView for the legacy
    // viewport.
    unsigned int displayStyle = MHWRender::MFrameContext::DisplayStyle::kGouraudShaded;

    if (vp2Context) {
        displayStyle = vp2Context->getDisplayStyle();
    } else if (view3d) {
        const M3dView::DisplayStyle legacyDisplayStyle = view3d->displayStyle();
        displayStyle = px_LegacyViewportUtils::GetMFrameContextDisplayStyle(legacyDisplayStyle);
    }

    // Since we'll be populating the prim filters with shape adapters, we don't
    // need to specify collections or render tags on them, so just use empty
    // ones.
    static const HdRprimCollection emptyCollection;
    static const TfTokenVector     emptyRenderTags;

    bool                     itemsVisible = false;
    std::vector<_RenderItem> items;
    for (const auto& iter : bucketsMap) {
        const PxrMayaHdRenderParams& params = iter.second.first;
        const _ShapeAdapterSet&      shapeAdapters = iter.second.second;

        PxrMayaHdPrimFilterVector primFilters;
        for (PxrMayaHdShapeAdapter* shapeAdapter : shapeAdapters) {
            shapeAdapter->UpdateVisibility(view3d);
            itemsVisible |= shapeAdapter->IsVisible();

            primFilters.push_back(
                PxrMayaHdPrimFilter { shapeAdapter, emptyCollection, emptyRenderTags });
        }

        items.push_back(std::make_pair(params, std::move(primFilters)));
    }

    if (!itemsVisible) {
        TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
            .Msg("    *** No objects visible.\n"
                 "    ^^^^^^^^^^^^ RENDER STAGE FINISH ^^^^^^^^^^^^^\n");
        return;
    }

    // Update lighting depending on VP2/Legacy.
    if (vp2Context) {
        _taskDelegate->SetLightingStateFromMayaDrawContext(*vp2Context);
    } else {
        // Maya does not appear to use GL_LIGHT_MODEL_AMBIENT, but it leaves
        // the default value of (0.2, 0.2, 0.2, 1.0) in place. The first time
        // that the viewport is set to use lights in the scene (instead of the
        // default lights or the no/flat lighting modes), the value is reset to
        // (0.0, 0.0, 0.0, 1.0), and it does not get reverted if/when the
        // lighting mode is changed back.
        // Since in the legacy viewport we get the lighting context from
        // OpenGL, we read in GL_LIGHT_MODEL_AMBIENT as the scene ambient. We
        // therefore need to explicitly set GL_LIGHT_MODEL_AMBIENT to the
        // zero/no ambient value before we do, otherwise we would end up using
        // the "incorrect" (i.e. not what Maya itself uses) default value.
        // This is not a problem in Viewport 2.0, since we do not consult
        // OpenGL at all for any of the lighting context state.
        glPushAttrib(GL_LIGHTING_BIT);

        const GfVec4f zeroAmbient(0.0f, 0.0f, 0.0f, 1.0f);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zeroAmbient.data());

        _taskDelegate->SetLightingStateFromVP1(worldToViewMatrix, projectionMatrix);

        glPopAttrib(); // GL_LIGHTING_BIT
    }

    _Render(worldToViewMatrix, projectionMatrix, viewport, displayStyle, items);

    // Viewport 2 may be rendering in multiple passes, and we want to make sure
    // we draw once (and only once) for each of those passes, so we delay
    // swapping the render queue into the select queue until we receive a
    // notification that all rendering has ended.
    // For the legacy viewport, rendering is done in a single pass and we will
    // not receive a notification at the end of rendering, so we do the swap
    // now.
    if (!vp2Context) {
        _MayaRenderDidEnd(nullptr);
    }

    TF_DEBUG(PXRUSDMAYAGL_BATCHED_DRAWING)
        .Msg("    ^^^^^^^^^^^^ RENDER STAGE FINISH ^^^^^^^^^^^^^\n");
}

bool UsdMayaGLBatchRenderer::_UpdateIsSelectionPending(const bool isPending)
{
    if (_isSelectionPending == isPending) {
        return false;
    }

    _isSelectionPending = isPending;

    return true;
}

void UsdMayaGLBatchRenderer::StartBatchingFrameDiagnostics()
{
    if (!_sharedDiagBatchCtx) {
        _sharedDiagBatchCtx.reset(new UsdMayaDiagnosticBatchContext());
    }
}

void UsdMayaGLBatchRenderer::_MayaRenderDidEnd(const MHWRender::MDrawContext* /* context */)
{
    // Completing a viewport render invalidates any previous selection
    // computation we may have done, so mark a new one as pending.
    _UpdateIsSelectionPending(true);

    // End any diagnostics batching.
    _sharedDiagBatchCtx.reset();
}

PXR_NAMESPACE_CLOSE_SCOPE
