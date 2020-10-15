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
#ifndef MAYAUSD_PROXYSTAGE_NOTICE_H
#define MAYAUSD_PROXYSTAGE_NOTICE_H

#include <mayaUsd/base/api.h>

#include <pxr/base/tf/notice.h>

#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

class MayaUsdProxyShapeBase;

/// Notice sent when the ProxyShape loads a new stage
class MayaUsdProxyStageBaseNotice : public TfNotice
{
public:
    MAYAUSD_CORE_PUBLIC
    MayaUsdProxyStageBaseNotice(const MayaUsdProxyShapeBase& proxy);

    /// Get proxy shape which had stage set
    MAYAUSD_CORE_PUBLIC
    const MayaUsdProxyShapeBase& GetProxyShape() const;

private:
    const MayaUsdProxyShapeBase& _proxy;
};

class MayaUsdProxyStageSetNotice : public MayaUsdProxyStageBaseNotice
{
public:
    using MayaUsdProxyStageBaseNotice::MayaUsdProxyStageBaseNotice;
};

class MayaUsdProxyStageInvalidateNotice : public MayaUsdProxyStageBaseNotice
{
public:
    using MayaUsdProxyStageBaseNotice::MayaUsdProxyStageBaseNotice;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
