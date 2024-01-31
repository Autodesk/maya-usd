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
#ifndef MAYAUSD_API_PROXYSTAGE_NOTICE_H
#define MAYAUSD_API_PROXYSTAGE_NOTICE_H

#include <mayaUsdAPI/api.h>

#include <pxr/base/tf/notice.h>
#include <pxr/usd/usd/notice.h>
#include <pxr/usd/usd/stage.h>

#include <maya/MObject.h>

namespace MAYAUSDAPI_NS_DEF {

/** \brief Base class for all Notices related to Usd stages events
 *   Context : a mayaUsd proxy shape node is an MpxNode subclass from mayaUsd which holds a USD
 * stage. These notices are made to listen to certain events that happen on the stage. \class
 * ProxyStageBaseNotice
 */
class ProxyStageBaseNotice : public PXR_NS::TfNotice
{
public:
    //! Constructor
    MAYAUSD_API_PUBLIC
    ProxyStageBaseNotice(MObject proxyObj);

    //! Get the proxy shape MObject
    MAYAUSD_API_PUBLIC
    MObject GetProxyShapeObj() const;

    //! Get the proxy shape Maya::MDagPath as a string
    MAYAUSD_API_PUBLIC
    std::string GetProxyShapePath() const;

    //! Get the usd stage
    MAYAUSD_API_PUBLIC
    PXR_NS::UsdStageRefPtr GetProxyShapeStage() const;

private:
    //! The mayaUsd proxy shape MObject
    MObject _proxyObj;
};

//! \brief Notice sent when the Usd stage is set in the mayaUsd proxy shape node
//! \class ProxyStageBaseNotice
class ProxyStageSetNotice : public ProxyStageBaseNotice
{
public:
    using ProxyStageBaseNotice::ProxyStageBaseNotice;
};

//! \brief Notice sent when the Usd stage is invalidated in the mayaUsd proxy shape node
//! \class ProxyStageBaseNotice
class ProxyStageInvalidateNotice : public ProxyStageBaseNotice
{
public:
    using ProxyStageBaseNotice::ProxyStageBaseNotice;
};

//! \brief Notice sent when some objects changed in the Usd stage from the mayaUsd proxy shape node
//! \class ProxyStageBaseNotice
class ProxyStageObjectsChangedNotice : public ProxyStageBaseNotice
{
public:
    //! Constructor
    MAYAUSD_API_PUBLIC
    ProxyStageObjectsChangedNotice(
        MObject                                  proxyObj,
        const PXR_NS::UsdNotice::ObjectsChanged& notice);

    //! Get the UsdNotice::ObjectsChanged
    MAYAUSD_API_PUBLIC
    const PXR_NS::UsdNotice::ObjectsChanged& GetNotice() const;

private:
    //! The UsdNotice::ObjectsChanged
    const PXR_NS::UsdNotice::ObjectsChanged& _notice;
};

} // End of namespace MAYAUSDAPI_NS_DEF

#endif // MAYAUSD_API_PROXYSTAGE_NOTICE_H
