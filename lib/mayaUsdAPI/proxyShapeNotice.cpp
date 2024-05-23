//
// Copyright 2023 Autodesk
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

#include "proxyStage.h"

#include <mayaUsd/listeners/proxyShapeNotice.h>
#include <mayaUsd/nodes/proxyShapeBase.h>

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/instantiateType.h>

#include <maya/MDagPath.h>

namespace MAYAUSDAPI_NS_DEF {

ProxyStageBaseNotice::ProxyStageBaseNotice(MObject proxyObj)
    : _proxyObj(proxyObj)
{
}

MObject ProxyStageBaseNotice::GetProxyShapeObj() const { return _proxyObj; }

std::string ProxyStageBaseNotice::GetProxyShapePath() const
{
    std::string path;
    MDagPath    shapeDagPath;
    if (MDagPath::getAPathTo(_proxyObj, shapeDagPath)) {
        path = shapeDagPath.fullPathName().asChar();
    }
    return path;
}

UsdStageRefPtr ProxyStageBaseNotice::GetProxyShapeStage() const
{
    ProxyStage proxyStage = ProxyStage(_proxyObj);
    return proxyStage.getUsdStage();
}

ProxyStageObjectsChangedNotice::ProxyStageObjectsChangedNotice(
    MObject                          proxyObj,
    const UsdNotice::ObjectsChanged& notice)
    : ProxyStageBaseNotice(proxyObj)
    , _notice(notice)
{
}

const UsdNotice::ObjectsChanged& ProxyStageObjectsChangedNotice::GetNotice() const
{
    return _notice;
}

TF_INSTANTIATE_TYPE(ProxyStageBaseNotice, TfType::CONCRETE, TF_1_PARENT(TfNotice));
TF_INSTANTIATE_TYPE(ProxyStageSetNotice, TfType::CONCRETE, TF_1_PARENT(ProxyStageBaseNotice));
TF_INSTANTIATE_TYPE(
    ProxyStageInvalidateNotice,
    TfType::CONCRETE,
    TF_1_PARENT(ProxyStageBaseNotice));
TF_INSTANTIATE_TYPE(
    ProxyStageObjectsChangedNotice,
    TfType::CONCRETE,
    TF_1_PARENT(ProxyStageBaseNotice));

//! Singleton class to listen to all the notices related to the mayaUsd proxy shape usd stage and
//! forward them with the Notices from the mayaUsdAPI
class MayaUsdProxyShapeNoticeListener : public TfWeakBase
{
public:
    //! Constructor
    MayaUsdProxyShapeNoticeListener()
    {
        TfSingleton<MayaUsdProxyShapeNoticeListener>::SetInstanceConstructed(*this);
        TfWeakPtr<MayaUsdProxyShapeNoticeListener> ptr(this);
        TfNotice::Register(ptr, &MayaUsdProxyShapeNoticeListener::_StageSet);
        TfNotice::Register(ptr, &MayaUsdProxyShapeNoticeListener::_StageInvalidate);
        TfNotice::Register(ptr, &MayaUsdProxyShapeNoticeListener::_ObjectsChanged);
    }

    //! Singleton access
    static MayaUsdProxyShapeNoticeListener& GetInstance()
    {
        return TfSingleton<MayaUsdProxyShapeNoticeListener>::GetInstance();
    }

private:
    //! Listen to the MayaUsdProxyStageBaseNotice and forward the mayaUsdAPI ProxyStageSetNotice
    void _StageSet(const MayaUsdProxyStageSetNotice& notice)
    {
        ProxyStageSetNotice(notice.GetProxyShape().thisMObject()).Send();
    }

    //! Listen to the MayaUsdProxyStageInvalidateNotice and forward the mayaUsdAPI
    //! ProxyStageInvalidateNotice
    void _StageInvalidate(const MayaUsdProxyStageInvalidateNotice& notice)
    {
        ProxyStageInvalidateNotice(notice.GetProxyShape().thisMObject()).Send();
    }

    //! Listen to the MayaUsdProxyStageObjectsChangedNotice and forward the mayaUsdAPI
    //! ProxyStageObjectsChangedNotice
    void _ObjectsChanged(const MayaUsdProxyStageObjectsChangedNotice& notice)
    {
        ProxyStageObjectsChangedNotice(notice.GetProxyShape().thisMObject(), notice.GetNotice())
            .Send();
    }
};

// Create the singleton that starts listening to mayaUsd proxy shape notices
MayaUsdProxyShapeNoticeListener _instance
    = TfSingleton<MayaUsdProxyShapeNoticeListener>::GetInstance();

} // End of namespace MAYAUSDAPI_NS_DEF

PXR_NAMESPACE_OPEN_SCOPE
TF_INSTANTIATE_SINGLETON(MayaUsdAPI::MayaUsdProxyShapeNoticeListener);
PXR_NAMESPACE_CLOSE_SCOPE
