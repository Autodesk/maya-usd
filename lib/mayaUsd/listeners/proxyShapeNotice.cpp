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
#include "proxyShapeNotice.h"

#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/tf/instantiateType.h>

#include <maya/MDagPath.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_TYPE(MayaUsdProxyStageSetNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));
TF_INSTANTIATE_TYPE(MayaUsdProxyStageInvalidateNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));
TF_INSTANTIATE_TYPE(MayaUsdProxyStageObjectsChangedNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));

MayaUsdProxyStageBaseNotice::MayaUsdProxyStageBaseNotice(const MayaUsdProxyShapeBase& proxy)
    : _proxy(proxy)
{
}

const MayaUsdProxyShapeBase& MayaUsdProxyStageBaseNotice::GetProxyShape() const { return _proxy; }

const std::string MayaUsdProxyStageBaseNotice::GetShapePath() const
{
    std::string path;
    MDagPath    shapeDagPath;
    if (MDagPath::getAPathTo(_proxy.thisMObject(), shapeDagPath)) {
        path = shapeDagPath.fullPathName().asChar();
    }
    return path;
}

UsdStageRefPtr MayaUsdProxyStageBaseNotice::GetStage() const { return _proxy.getUsdStage(); }

MayaUsdProxyStageObjectsChangedNotice::MayaUsdProxyStageObjectsChangedNotice(
    const MayaUsdProxyShapeBase&     proxy,
    const UsdNotice::ObjectsChanged& notice)
    : MayaUsdProxyStageBaseNotice(proxy)
    , _notice(notice)
{
}

const UsdNotice::ObjectsChanged& MayaUsdProxyStageObjectsChangedNotice::GetNotice() const
{
    return _notice;
}

PXR_NAMESPACE_CLOSE_SCOPE
