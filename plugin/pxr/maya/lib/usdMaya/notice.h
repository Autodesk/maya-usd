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
#ifndef USDMAYA_NOTICE_H
#define USDMAYA_NOTICE_H

/// \file usdMaya/notice.h

#include "pxr/base/tf/notice.h"

#include "usdMaya/api.h"

#include <maya/MMessage.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Notice sent when the Maya scene resets, either by opening a new scene or
/// switching to a new scene.
/// It is *very important* that you call InstallListener() during plugin
/// initialization and removeListener() during plugin uninitialization.
class UsdMayaSceneResetNotice : public TfNotice
{
public:
    PXRUSDMAYA_API
    UsdMayaSceneResetNotice();

    /// Registers the proper Maya callbacks for recognizing stage resets.
    PXRUSDMAYA_API
    static void InstallListener();

    /// Removes any Maya callbacks for recognizing stage resets.
    PXRUSDMAYA_API
    static void RemoveListener();

private:
    static MCallbackId _afterNewCallbackId;
    static MCallbackId _beforeFileReadCallbackId;
};

class UsdMaya_AssemblyInstancerNoticeBase : public TfNotice
{
public:
    PXRUSDMAYA_API
    virtual ~UsdMaya_AssemblyInstancerNoticeBase() = default;

    PXRUSDMAYA_API
    MObject GetAssembly() const;

    PXRUSDMAYA_API
    MObject GetInstancer() const;

protected:
    PXRUSDMAYA_API
    UsdMaya_AssemblyInstancerNoticeBase(
            const MObject& assembly,
            const MObject& instancer);

private:
    MObject _assembly;
    MObject _instancer;
};

/// Notice sent when any reference assembly is connected as a prototype of a
/// native Maya instancer.
class UsdMayaAssemblyConnectedToInstancerNotice
        : public UsdMaya_AssemblyInstancerNoticeBase
{
public:
    PXRUSDMAYA_API
    UsdMayaAssemblyConnectedToInstancerNotice(
            const MObject& assembly,
            const MObject& instancer);
};

/// Notice sent when any reference assembly was previously a prototype of a
/// native Maya instancer but has now been disconnected from it.
class UsdMayaAssemblyDisconnectedFromInstancerNotice
        : public UsdMaya_AssemblyInstancerNoticeBase
{
public:
    PXRUSDMAYA_API
    UsdMayaAssemblyDisconnectedFromInstancerNotice(
            const MObject& assembly,
            const MObject& instancer);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
