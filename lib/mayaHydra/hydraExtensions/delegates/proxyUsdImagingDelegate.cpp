//
// Copyright 2019 Luma Pictures
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
#include "proxyUsdImagingDelegate.h"

#include <mayaUsd/nodes/proxyShapeBase.h>

#include <maya/MMatrix.h>

PXR_NAMESPACE_OPEN_SCOPE

HdMayaProxyUsdImagingDelegate::HdMayaProxyUsdImagingDelegate(
    HdRenderIndex*         parentIndex,
    SdfPath const&         delegateID,
    MayaUsdProxyShapeBase* proxy,
    const MDagPath&        dagPath)
    : UsdImagingDelegate(parentIndex, delegateID)
    , _dagPath(dagPath)
    , _proxy(proxy)
{
    // TODO Auto-generated constructor stub
}

HdMayaProxyUsdImagingDelegate::~HdMayaProxyUsdImagingDelegate()
{
    // TODO Auto-generated destructor stub
}

GfMatrix4d HdMayaProxyUsdImagingDelegate::GetTransform(SdfPath const& id)
{
    if (_rootTransformDirty) {
        UpdateRootTransform();
    }
    return UsdImagingDelegate::GetTransform(id);
}

bool HdMayaProxyUsdImagingDelegate::GetVisible(SdfPath const& id)
{
    if (_rootVisibilityDirty) {
        UpdateRootVisibility();
    }
    return UsdImagingDelegate::GetVisible(id);
}

void HdMayaProxyUsdImagingDelegate::UpdateRootTransform()
{
    SetRootTransform(GfMatrix4d(_proxy->parentTransform().inclusiveMatrix().matrix));
    _rootTransformDirty = false;
}

void HdMayaProxyUsdImagingDelegate::UpdateRootVisibility()
{
    SetRootVisibility(_dagPath.isVisible());
    _rootVisibilityDirty = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
