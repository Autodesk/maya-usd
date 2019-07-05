//
// Copyright 2018 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USDMAYA_NOTICE_H
#define USDMAYA_NOTICE_H

/// \file usdMaya/notice.h

#include "pxr/base/tf/notice.h"

#include "../base/api.h"

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
    MAYAUSD_CORE_PUBLIC
    UsdMayaSceneResetNotice();

    /// Registers the proper Maya callbacks for recognizing stage resets.
    MAYAUSD_CORE_PUBLIC
    static void InstallListener();

    /// Removes any Maya callbacks for recognizing stage resets.
    MAYAUSD_CORE_PUBLIC
    static void RemoveListener();

private:
    static MCallbackId _afterNewCallbackId;
    static MCallbackId _beforeFileReadCallbackId;
};

class UsdMaya_AssemblyInstancerNoticeBase : public TfNotice
{
public:
    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMaya_AssemblyInstancerNoticeBase() = default;

    MAYAUSD_CORE_PUBLIC
    MObject GetAssembly() const;

    MAYAUSD_CORE_PUBLIC
    MObject GetInstancer() const;

protected:
    MAYAUSD_CORE_PUBLIC
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
    MAYAUSD_CORE_PUBLIC
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
    MAYAUSD_CORE_PUBLIC
    UsdMayaAssemblyDisconnectedFromInstancerNotice(
            const MObject& assembly,
            const MObject& instancer);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
