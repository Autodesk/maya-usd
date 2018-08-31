//
// Copyright 2018 Luma Pictures
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http:#www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include <pxr/pxr.h>

#include <pxr/base/tf/type.h>

#include <pxr/usd/usdGeom/tokens.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/imaging/pxOsd/tokens.h>

#include <maya/MFnNurbsCurve.h>

#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/adapters/adapterRegistry.h>
#include <hdmaya/adapters/mayaAttrs.h>
#include <hdmaya/adapters/shapeAdapter.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaNurbsCurveAdapter : public HdMayaShapeAdapter {
public:
    HdMayaNurbsCurveAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaShapeAdapter(delegate->GetPrimPath(dag), delegate, dag) {}

    ~HdMayaNurbsCurveAdapter() = default;

    bool IsSupported() override {
        return GetDelegate()->GetRenderIndex().IsRprimTypeSupported(
            HdPrimTypeTokens->basisCurves);
    }

    void Populate() override {
        GetDelegate()->InsertRprim(
            HdPrimTypeTokens->basisCurves, GetID(), HdChangeTracker::AllDirty);
    }
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaNurbsCurveAdapter, TfType::Bases<HdMayaShapeAdapter>>();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, mesh) {
    HdMayaAdapterRegistry::RegisterShapeAdapter(
        TfToken("nurbsCurve"),
        [](HdMayaDelegateCtx* delegate,
           const MDagPath& dag) -> HdMayaShapeAdapterPtr {
            return HdMayaShapeAdapterPtr(
                new HdMayaNurbsCurveAdapter(delegate, dag));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
