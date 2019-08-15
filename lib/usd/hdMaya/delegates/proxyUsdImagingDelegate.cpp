//
// Copyright 2019 Luma Pictures
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

#include "proxyUsdImagingDelegate.h"

#include "../../../nodes/proxyShapeBase.h"

#include <maya/MMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

HdMayaProxyUsdImagingDelegate::HdMayaProxyUsdImagingDelegate(
    HdRenderIndex* parentIndex, SdfPath const& delegateID,
    MayaUsdProxyShapeBase* proxy, const MDagPath& dagPath)
    : UsdImagingDelegate(parentIndex, delegateID),
      _dagPath(dagPath),
      _proxy(proxy) {
    // TODO Auto-generated constructor stub
}

HdMayaProxyUsdImagingDelegate::~HdMayaProxyUsdImagingDelegate() {
    // TODO Auto-generated destructor stub
}

GfMatrix4d HdMayaProxyUsdImagingDelegate::GetTransform(SdfPath const& id) {
    if (_rootTransformDirty) { UpdateRootTransform(); }
    return UsdImagingDelegate::GetTransform(id);
}

bool HdMayaProxyUsdImagingDelegate::GetVisible(SdfPath const& id) {
    if (_rootVisibilityDirty) { UpdateRootVisibility(); }
    return UsdImagingDelegate::GetVisible(id);
}

void HdMayaProxyUsdImagingDelegate::UpdateRootTransform() {
    SetRootTransform(
        GfMatrix4d(_proxy->parentTransform().inclusiveMatrix().matrix));
    _rootTransformDirty = false;
}

void HdMayaProxyUsdImagingDelegate::UpdateRootVisibility() {
    SetRootVisibility(_dagPath.isVisible());
    _rootVisibilityDirty = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
