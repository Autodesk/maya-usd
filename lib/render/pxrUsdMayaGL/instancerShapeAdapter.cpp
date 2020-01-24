//
// Copyright 2018 Pixar
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
#include "./instancerShapeAdapter.h"

#include "./batchRenderer.h"
#include "./debugCodes.h"
#include "./renderParams.h"
#include "./shapeAdapter.h"

#include "../../utils/util.h"
#include "../../fileio/utils/writeUtil.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/kind/registry.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/usd/usdGeom/pointInstancer.h"

#include "pxr/usdImaging/usdImaging/delegate.h"

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <boost/functional/hash.hpp>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((NativeInstancerType, "instancer"))
    (Instancer)
    (Prototypes)
    (EmptyPrim)
);

/* virtual */
bool
UsdMayaGL_InstancerShapeAdapter::UpdateVisibility(const M3dView* view)
{
    bool isVisible;
    if (!_GetVisibility(_shapeDagPath, view, &isVisible)) {
        return false;
    }

    if (_delegate && _delegate->GetRootVisibility() != isVisible) {
        _delegate->SetRootVisibility(isVisible);
        return true;
    }

    return false;
}

/* virtual */
bool
UsdMayaGL_InstancerShapeAdapter::IsVisible() const
{
    return (_delegate && _delegate->GetRootVisibility());
}

/* virtual */
void
UsdMayaGL_InstancerShapeAdapter::SetRootXform(const GfMatrix4d& transform)
{
    _rootXform = transform;

    if (_delegate) {
        _delegate->SetRootTransform(_rootXform);
    }
}

/* virtual */
const SdfPath&
UsdMayaGL_InstancerShapeAdapter::GetDelegateID() const
{
    if (_delegate) {
        return _delegate->GetDelegateID();
    }

    return SdfPath::EmptyPath();
}

static void
_ClearInstancer(const UsdGeomPointInstancer& usdInstancer)
{
    usdInstancer.GetPrototypesRel().SetTargets({
    SdfPath::AbsoluteRootPath()
            .AppendChild(_tokens->Instancer)
            .AppendChild(_tokens->EmptyPrim)});
    usdInstancer.CreateProtoIndicesAttr().Set(VtIntArray());
    usdInstancer.CreatePositionsAttr().Set(VtVec3fArray());
    usdInstancer.CreateOrientationsAttr().Set(VtQuathArray());
    usdInstancer.CreateScalesAttr().Set(VtVec3fArray());
}

size_t
UsdMayaGL_InstancerShapeAdapter::_SyncInstancerPrototypes(
        const UsdGeomPointInstancer& usdInstancer,
        const MPlug& inputHierarchy)
{
    usdInstancer.GetPrototypesRel().ClearTargets(/*removeSpec*/ false);

    // Write prototypes using a custom code path. We're only going to
    // export USD reference assemblies; any native objects will be left
    // as empty prims.
    const UsdStagePtr stage = usdInstancer.GetPrim().GetStage();
    stage->MuteAndUnmuteLayers({}, stage->GetMutedLayers());

    const SdfPath prototypesGroupPath = SdfPath::AbsoluteRootPath()
            .AppendChild(_tokens->Instancer)
            .AppendChild(_tokens->Prototypes);
    std::vector<std::string> layerIdsToMute;
    for (unsigned int i = 0; i < inputHierarchy.numElements(); ++i) {
        // Set up an empty prim for the prototype reference.
        // This code path is designed so that, after setting up the prim,
        // we can just leave it and "continue" if we error trying to set it up.
        const TfToken prototypeName(TfStringPrintf("prototype_%d", i));
        const SdfPath prototypeUsdPath =
                prototypesGroupPath.AppendChild(prototypeName);
        UsdPrim prototypePrim = stage->DefinePrim(prototypeUsdPath);
        UsdModelAPI(prototypePrim).SetKind(KindTokens->component);
        usdInstancer.GetPrototypesRel().AddTarget(prototypeUsdPath);

        SyncInstancerPerPrototypePostHook(
            inputHierarchy[i], prototypePrim, layerIdsToMute);
    }

    // Actually do all the muting in a batch.
    stage->MuteAndUnmuteLayers(layerIdsToMute, {});

    return inputHierarchy.numElements();
}

void
UsdMayaGL_InstancerShapeAdapter::_SyncInstancer(
        const UsdGeomPointInstancer& usdInstancer,
        const MDagPath& mayaInstancerPath)
{
    MStatus status;
    MFnDagNode dagNode(mayaInstancerPath, &status);
    if (!status) {
        _ClearInstancer(usdInstancer);
        return;
    }

    MPlug inputPoints = dagNode.findPlug("inputPoints", &status);
    if (!status) {
        _ClearInstancer(usdInstancer);
        return;
    }

    MPlug inputHierarchy = dagNode.findPlug("inputHierarchy", &status);
    if (!status) {
        _ClearInstancer(usdInstancer);
        return;
    }

    MPlug inputPointsSrc = UsdMayaUtil::GetConnected(inputPoints);
    if (inputPointsSrc.isNull()) {
        _ClearInstancer(usdInstancer);
        return;
    }

    auto holder = UsdMayaUtil::GetPlugDataHandle(inputPointsSrc);
    if (!holder) {
        _ClearInstancer(usdInstancer);
        return;
    }

    MFnArrayAttrsData data(holder->GetDataHandle().data(), &status);
    if (!status) {
        _ClearInstancer(usdInstancer);
        return;
    }

    size_t numPrototypes = _SyncInstancerPrototypes(
            usdInstancer, inputHierarchy);
    if (!numPrototypes) {
        _ClearInstancer(usdInstancer);
        return;
    }

    // Write PointInstancer attrs using export code path.
    UsdMayaWriteUtil::WriteArrayAttrsToInstancer(
            data, usdInstancer, numPrototypes,
            UsdTimeCode::Default());
}

/* virtual */
bool
UsdMayaGL_InstancerShapeAdapter::_Sync(
        const MDagPath& shapeDagPath,
        const unsigned int displayStyle,
        const MHWRender::DisplayStatus displayStatus)
{
    MStatus status;
    UsdPrim usdPrim = _instancerStage->GetDefaultPrim();
    UsdGeomPointInstancer instancer(usdPrim);
    _SyncInstancer(instancer, shapeDagPath);

    // Check for updates to the shape or changes in the batch renderer that
    // require us to re-initialize the shape adapter.
    HdRenderIndex* renderIndex =
        UsdMayaGLBatchRenderer::GetInstance().GetRenderIndex();
    if (!(shapeDagPath == _shapeDagPath) ||
            !_delegate ||
            renderIndex != &_delegate->GetRenderIndex()) {
        _shapeDagPath = shapeDagPath;

        if (!_Init(renderIndex)) {
            return false;
        }
    }

    // Reset _renderParams to the defaults.
    _renderParams = PxrMayaHdRenderParams();

    const MMatrix transform = _shapeDagPath.inclusiveMatrix(&status);
    if (status == MS::kSuccess) {
        _rootXform = GfMatrix4d(transform.matrix);
        _delegate->SetRootTransform(_rootXform);
    }

    _delegate->SetTime(UsdTimeCode::EarliestTime());

    // In contrast with the other shape adapters, this adapter ignores the
    // selection wireframe. The native instancer doesn't draw selection
    // wireframes, so we want to mimic that behavior for consistency.
    HdReprSelector reprSelector =
        GetReprSelectorForDisplayState(
            displayStyle,
            displayStatus);

    _drawShape = reprSelector.AnyActiveRepr();

    // We won't ever draw the bounding box here because the native Maya
    // instancer already draws a bounding box, and we don't want to draw two.
    // XXX: The native Maya instancer's bounding box will only cover the native
    // geometry, though; is there any way to "teach" it about our bounds?
    _drawBoundingBox = false;

    // If the repr selector specifies a wireframe-only repr, then disable
    // lighting.
    if (reprSelector.Contains(HdReprTokens->wire) ||
            reprSelector.Contains(HdReprTokens->refinedWire)) {
        _renderParams.enableLighting = false;
    }

    if (_delegate->GetRootVisibility() != _drawShape) {
        _delegate->SetRootVisibility(_drawShape);
    }

    if (_rprimCollection.GetReprSelector() != reprSelector) {
        _rprimCollection.SetReprSelector(reprSelector);

        TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
                "    Repr selector changed: %s\n"
                "        Marking collection dirty: %s\n",
                reprSelector.GetText(),
                _rprimCollection.GetName().GetText());

        _delegate->GetRenderIndex().GetChangeTracker().MarkCollectionDirty(
            _rprimCollection.GetName());
    }

    // The kBackfaceCulling display style was introduced in Maya 2016 SP2.
    HdCullStyle cullStyle = HdCullStyleNothing;
#if MAYA_API_VERSION >= 201603
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        cullStyle = HdCullStyleBackUnlessDoubleSided;
    }
#endif

    _delegate->SetCullStyleFallback(cullStyle);

    return true;
}

bool
UsdMayaGL_InstancerShapeAdapter::_Init(HdRenderIndex* renderIndex)
{
    if (!TF_VERIFY(renderIndex,
                   "Cannot initialize shape adapter with invalid HdRenderIndex")) {
        return false;
    }

    const SdfPath delegatePrefix =
        UsdMayaGLBatchRenderer::GetInstance().GetDelegatePrefix(_isViewport2);

    // Create a simple "name" for this shape adapter to insert into the batch
    // renderer's SdfPath hierarchy.
    //
    // XXX: For as long as we're using the MAYA_VP2_USE_VP1_SELECTION
    // environment variable, we need to be able to pass responsibility back and
    // forth between the MPxDrawOverride's shape adapter for drawing and the
    // MPxSurfaceShapeUI's shape adapter for selection. This requires both
    // shape adapters to have the same "name", which forces us to build it
    // from data on the shape that will be common to both classes, as we do
    // below. When we remove MAYA_VP2_USE_VP1_SELECTION and can trust that a
    // single shape adapter handles both drawing and selection, we can do
    // something even simpler instead like using the shape adapter's memory
    // address as the "name".
    size_t shapeHash(MObjectHandle(_shapeDagPath.transform()).hashCode());
    boost::hash_combine(shapeHash, _instancerStage->GetDefaultPrim());

    // We prepend the Maya type name to the beginning of the delegate name to
    // ensure that there are no name collisions between shape adapters of
    // shapes with different Maya types.
    const TfToken delegateName(
        TfStringPrintf("%s_%zx",
                       _tokens->NativeInstancerType.GetText(),
                       shapeHash));

    const SdfPath delegateId = delegatePrefix.AppendChild(delegateName);

    if (_delegate &&
            delegateId == GetDelegateID() &&
            renderIndex == &_delegate->GetRenderIndex()) {
        // The delegate's current ID matches the delegate ID we computed and
        // the render index matches, so it must be up to date already.
        return true;
    }

    const TfToken collectionName = _GetRprimCollectionName();

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Initializing UsdMayaGL_InstancerShapeAdapter: %p\n"
        "    shape DAG path : %s\n"
        "    collection name: %s\n"
        "    delegateId     : %s\n",
        this,
        _shapeDagPath.fullPathName().asChar(),
        collectionName.GetText(),
        delegateId.GetText());

    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateId));
    if (!TF_VERIFY(_delegate,
                  "Failed to create shape adapter delegate for shape %s",
                  _shapeDagPath.fullPathName().asChar())) {
        return false;
    }

    UsdPrim usdPrim = _instancerStage->GetDefaultPrim();
    _delegate->Populate(usdPrim, SdfPathVector());

    if (collectionName != _rprimCollection.GetName()) {
        _rprimCollection.SetName(collectionName);
        renderIndex->GetChangeTracker().AddCollection(
                _rprimCollection.GetName());
    }

    _rprimCollection.SetReprSelector(HdReprSelector(HdReprTokens->refined));
    _rprimCollection.SetRootPath(delegateId);

    return true;
}

/* virtual */
void UsdMayaGL_InstancerShapeAdapter::SyncInstancerPerPrototypePostHook(
    const MPlug&              ,
    UsdPrim&                  prototypePrim,
    std::vector<std::string>& 
)
{
    UsdReferences prototypeRefs = prototypePrim.GetReferences();
    prototypeRefs.ClearReferences();
}

UsdMayaGL_InstancerShapeAdapter::UsdMayaGL_InstancerShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Constructing UsdMayaGL_InstancerShapeAdapter: %p\n",
        this);

    // Set up bare-bones instancer stage.
    // Populate the required properties for the instancer.
    _instancerStage = UsdStage::CreateInMemory();
    const SdfPath instancerPath =
            SdfPath::AbsoluteRootPath().AppendChild(_tokens->Instancer);
    const SdfPath prototypesPath =
            instancerPath.AppendChild(_tokens->Prototypes);
    const SdfPath emptyPrimPath =
            instancerPath.AppendChild(_tokens->EmptyPrim);
    const UsdGeomPointInstancer instancer =
            UsdGeomPointInstancer::Define(_instancerStage, instancerPath);
    const UsdPrim prototypesGroupPrim =
            _instancerStage->DefinePrim(prototypesPath);
    const UsdPrim emptyPrim =
            _instancerStage->DefinePrim(emptyPrimPath);
    instancer.CreatePrototypesRel().AddTarget(emptyPrimPath);
    instancer.CreateProtoIndicesAttr().Set(VtIntArray());
    instancer.CreatePositionsAttr().Set(VtVec3fArray());
    instancer.CreateOrientationsAttr().Set(VtQuathArray());
    instancer.CreateScalesAttr().Set(VtVec3fArray());
    UsdModelAPI(instancer).SetKind(KindTokens->assembly);
    UsdModelAPI(prototypesGroupPrim).SetKind(KindTokens->group);
    _instancerStage->SetDefaultPrim(instancer.GetPrim());
}

/* virtual */
UsdMayaGL_InstancerShapeAdapter::~UsdMayaGL_InstancerShapeAdapter()
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE).Msg(
        "Destructing UsdMayaGL_InstancerShapeAdapter: %p\n",
        this);
}


PXR_NAMESPACE_CLOSE_SCOPE
