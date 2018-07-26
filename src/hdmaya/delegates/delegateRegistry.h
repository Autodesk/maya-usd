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
#ifndef __HDMAYA_DELEGATE_REGISTRY_H__
#define __HDMAYA_DELEGATE_REGISTRY_H__

#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/sdf/path.h>

#include <tuple>
#include <vector>

#include <hdmaya/delegates/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDelegateRegistry : public TfSingleton<HdMayaDelegateRegistry> {
    friend class TfSingleton<HdMayaDelegateRegistry>;
    HDMAYA_API
    HdMayaDelegateRegistry() = default;

public:
    // function creates and returns a pointer to a HdMayaDelegate - may return
    // a nullptr indicate failure, or that the delegate is currently disabled
    using DelegateCreator = std::function<HdMayaDelegatePtr(HdRenderIndex*, const SdfPath&)>;

    HDMAYA_API
    static void RegisterDelegate(const TfToken& name, DelegateCreator creator);
    HDMAYA_API
    static std::vector<TfToken> GetDelegateNames();
    HDMAYA_API
    static std::vector<DelegateCreator> GetDelegateCreators();

    // Signal that some delegate types are now either valid or invalid.
    // ie, say some delegate type is only useful / works when a certain maya
    // plugin is loaded - you would call this every time that plugin was loaded
    // or unloaded.
    HDMAYA_API
    static void SignalDelegatesChanged();

    // Find all HdMayaDelegate plugins, and load them all
    HDMAYA_API
    static void LoadAllDelegates();

private:
    static void _LoadAllDelegates();

    std::vector<std::tuple<TfToken, DelegateCreator>> _delegates;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DELEGATE_REGISTRY_H__
