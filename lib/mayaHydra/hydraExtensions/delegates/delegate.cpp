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
#include "delegate.h"

#include <pxr/base/gf/interval.h>
#include <pxr/base/tf/type.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) { TfType::Define<HdMayaDelegate>(); }

HdMayaDelegate::HdMayaDelegate(const InitData& initData)
    : _mayaDelegateID(initData.delegateID)
    , _name(initData.name)
    , _engine(initData.engine)
    , _taskController(initData.taskController)
    , _isHdSt(initData.isHdSt)
{
}

void HdMayaDelegate::SetParams(const HdMayaParams& params) { _params = params; }

void HdMayaDelegate::SetCameraForSampling(SdfPath const& camID) { _cameraPathForSampling = camID; }

GfInterval HdMayaDelegate::GetCurrentTimeSamplingInterval() const
{
    return GfInterval(_params.motionSampleStart, _params.motionSampleEnd);
}

PXR_NAMESPACE_CLOSE_SCOPE
