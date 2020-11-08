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
#include "instancerShapeAdapter.h"

#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/render/pxrUsdMayaGL/batchRenderer.h>
#include <mayaUsd/render/pxrUsdMayaGL/debugCodes.h>
#include <mayaUsd/render/pxrUsdMayaGL/renderParams.h>
#include <mayaUsd/render/pxrUsdMayaGL/shapeAdapter.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/staticTokens.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/enums.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/repr.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/pointInstancer.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/M3dView.h>
#include <maya/MColor.h>
#include <maya/MDagPath.h>
#include <maya/MFnArrayAttrsData.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MMatrix.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((NativeInstancerType, "instancer"))
    (Instancer)
    (Prototypes)
    (EmptyPrim)
);
// clang-format on

/* virtual */
bool UsdMayaGL_InstancerShapeAdapter::UpdateVisibility(const M3dView* view)
{
    bool isVisible;
    if (!_GetVisibility(GetDagPath(), view, &isVisible)) {
        return false;
    }

    if (_delegate && _delegate->GetRootVisibility() != isVisible) {
        _delegate->SetRootVisibility(isVisible);
        return true;
    }

    return false;
}

/* virtual */
bool UsdMayaGL_InstancerShapeAdapter::IsVisible() const
{
    return (_delegate && _delegate->GetRootVisibility());
}

/* virtual */
void UsdMayaGL_InstancerShapeAdapter::SetRootXform(const GfMatrix4d& transform)
{
    _rootXform = transform;

    if (_delegate) {
        _delegate->SetRootTransform(_rootXform);
    }
}

static void _ClearInstancer(const UsdGeomPointInstancer& usdInstancer)
{
    usdInstancer.GetPrototypesRel().SetTargets({ SdfPath::AbsoluteRootPath()
                                                     .AppendChild(_tokens->Instancer)
                                                     .AppendChild(_tokens->EmptyPrim) });
    usdInstancer.CreateProtoIndicesAttr().Set(VtIntArray());
    usdInstancer.CreatePositionsAttr().Set(VtVec3fArray());
    usdInstancer.CreateOrientationsAttr().Set(VtQuathArray());
    usdInstancer.CreateScalesAttr().Set(VtVec3fArray());
}

size_t UsdMayaGL_InstancerShapeAdapter::_SyncInstancerPrototypes(
    const UsdGeomPointInstancer& usdInstancer,
    const MPlug&                 inputHierarchy)
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
        const SdfPath prototypeUsdPath = prototypesGroupPath.AppendChild(prototypeName);
        UsdPrim       prototypePrim = stage->DefinePrim(prototypeUsdPath);
        UsdModelAPI(prototypePrim).SetKind(KindTokens->component);
        usdInstancer.GetPrototypesRel().AddTarget(prototypeUsdPath);

        SyncInstancerPerPrototypePostHook(inputHierarchy[i], prototypePrim, layerIdsToMute);
    }

    // Actually do all the muting in a batch.
    stage->MuteAndUnmuteLayers(layerIdsToMute, {});

    return inputHierarchy.numElements();
}

void UsdMayaGL_InstancerShapeAdapter::_SyncInstancer(
    const UsdGeomPointInstancer& usdInstancer,
    const MDagPath&              mayaInstancerPath)
{
    MStatus    status;
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

    size_t numPrototypes = _SyncInstancerPrototypes(usdInstancer, inputHierarchy);
    if (!numPrototypes) {
        _ClearInstancer(usdInstancer);
        return;
    }

    // Write PointInstancer attrs using export code path.
    UsdMayaWriteUtil::WriteArrayAttrsToInstancer(
        data, usdInstancer, numPrototypes, UsdTimeCode::Default());
}

/* virtual */
bool UsdMayaGL_InstancerShapeAdapter::_Sync(
    const MDagPath&    shapeDagPath,
    const unsigned int displayStyle,
    const MHWRender::DisplayStatus /* displayStatus */)
{
    MStatus               status;
    UsdPrim               usdPrim = _instancerStage->GetDefaultPrim();
    UsdGeomPointInstancer instancer(usdPrim);
    _SyncInstancer(instancer, shapeDagPath);

    // Check for updates to the shape or changes in the batch renderer that
    // require us to re-initialize the shape adapter.
    HdRenderIndex* renderIndex = UsdMayaGLBatchRenderer::GetInstance().GetRenderIndex();
    if (!(shapeDagPath == GetDagPath()) || !_delegate
        || renderIndex != &_delegate->GetRenderIndex()) {
        _SetDagPath(shapeDagPath);

        if (!_Init(renderIndex)) {
            return false;
        }
    }

    // Reset _renderParams to the defaults.
    _renderParams = PxrMayaHdRenderParams();

    const MMatrix transform = GetDagPath().inclusiveMatrix(&status);
    if (status == MS::kSuccess) {
        _rootXform = GfMatrix4d(transform.matrix);
        _delegate->SetRootTransform(_rootXform);
    }

    _delegate->SetTime(UsdTimeCode::EarliestTime());

    // In contrast with the other shape adapters, this adapter ignores the
    // selection wireframe. The native instancer doesn't draw selection
    // wireframes, so we want to mimic that behavior for consistency.

    // XXX: This is not technically correct. Since the display style can vary
    // per viewport, this decision of whether or not to enable lighting should
    // be delayed until when the repr for each viewport is known during batched
    // drawing. For now, the incorrectly shaded wireframe is not too offensive
    // though.
    //
    // If the repr selector specifies a wireframe-only repr, then disable
    // lighting.
    const HdReprSelector reprSelector = GetReprSelectorForDisplayStyle(displayStyle);
    if (reprSelector.Contains(HdReprTokens->wire)
        || reprSelector.Contains(HdReprTokens->refinedWire)) {
        _renderParams.enableLighting = false;
    }

    HdCullStyle cullStyle = HdCullStyleNothing;
    if (displayStyle & MHWRender::MFrameContext::DisplayStyle::kBackfaceCulling) {
        cullStyle = HdCullStyleBackUnlessDoubleSided;
    }

    _delegate->SetCullStyleFallback(cullStyle);

    return true;
}

bool UsdMayaGL_InstancerShapeAdapter::_Init(HdRenderIndex* renderIndex)
{
    if (!TF_VERIFY(renderIndex, "Cannot initialize shape adapter with invalid HdRenderIndex")) {
        return false;
    }

    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg(
            "Initializing UsdMayaGL_InstancerShapeAdapter: %p\n"
            "    shape DAG path  : %s\n"
            "    shape identifier: %s\n"
            "    delegateId      : %s\n",
            this,
            GetDagPath().fullPathName().asChar(),
            _shapeIdentifier.GetText(),
            _delegateId.GetText());

    _delegate.reset(new UsdImagingDelegate(renderIndex, _delegateId));
    if (!TF_VERIFY(
            _delegate,
            "Failed to create shape adapter delegate for shape %s",
            GetDagPath().fullPathName().asChar())) {
        return false;
    }

    UsdPrim usdPrim = _instancerStage->GetDefaultPrim();
    _delegate->Populate(usdPrim, SdfPathVector());

    return true;
}

/* virtual */
void UsdMayaGL_InstancerShapeAdapter::SyncInstancerPerPrototypePostHook(
    const MPlug&,
    UsdPrim& prototypePrim,
    std::vector<std::string>&)
{
    UsdReferences prototypeRefs = prototypePrim.GetReferences();
    prototypeRefs.ClearReferences();
}

UsdMayaGL_InstancerShapeAdapter::UsdMayaGL_InstancerShapeAdapter(bool isViewport2)
    : PxrMayaHdShapeAdapter(isViewport2)
{
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Constructing UsdMayaGL_InstancerShapeAdapter: %p\n", this);

    // Set up bare-bones instancer stage.
    // Populate the required properties for the instancer.
    _instancerStage = UsdStage::CreateInMemory();
    const SdfPath instancerPath = SdfPath::AbsoluteRootPath().AppendChild(_tokens->Instancer);
    const SdfPath prototypesPath = instancerPath.AppendChild(_tokens->Prototypes);
    const SdfPath emptyPrimPath = instancerPath.AppendChild(_tokens->EmptyPrim);
    const UsdGeomPointInstancer instancer
        = UsdGeomPointInstancer::Define(_instancerStage, instancerPath);
    const UsdPrim prototypesGroupPrim = _instancerStage->DefinePrim(prototypesPath);
    const UsdPrim emptyPrim = _instancerStage->DefinePrim(emptyPrimPath);
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
    TF_DEBUG(PXRUSDMAYAGL_SHAPE_ADAPTER_LIFECYCLE)
        .Msg("Destructing UsdMayaGL_InstancerShapeAdapter: %p\n", this);
}

PXR_NAMESPACE_CLOSE_SCOPE
