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
#ifndef MAYAHYDRALIB_DELEGATE_REGISTRY_H
#define MAYAHYDRALIB_DELEGATE_REGISTRY_H

#include <mayaHydraLib/delegates/delegate.h>

#include <pxr/base/tf/singleton.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <tuple>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class MayaHydraDelegateRegistry : public TfSingleton<MayaHydraDelegateRegistry>
{
    friend class TfSingleton<MayaHydraDelegateRegistry>;
    MAYAHYDRALIB_API
    MayaHydraDelegateRegistry() = default;

public:
    // function creates and returns a pointer to a MayaHydraDelegate - may return
    // a nullptr indicate failure, or that the delegate is currently disabled
    using DelegateCreator = std::function<MayaHydraDelegatePtr(const MayaHydraDelegate::InitData&)>;

    MAYAHYDRALIB_API
    static void RegisterDelegate(const TfToken& name, DelegateCreator creator);
    MAYAHYDRALIB_API
    static std::vector<TfToken> GetDelegateNames();
    MAYAHYDRALIB_API
    static std::vector<DelegateCreator> GetDelegateCreators();

    // Signal that some delegate types are now either valid or invalid.
    // ie, say some delegate type is only useful / works when a certain maya
    // plugin is loaded - you would call this every time that plugin was loaded
    // or unloaded.
    MAYAHYDRALIB_API
    static void SignalDelegatesChanged();

    // Find all MayaHydraDelegate plugins, and load them all
    MAYAHYDRALIB_API
    static void LoadAllDelegates();

    using DelegatesChangedSignal = std::function<void()>;

    MAYAHYDRALIB_API
    static void InstallDelegatesChangedSignal(DelegatesChangedSignal signal);

private:
    static void _LoadAllDelegates();

    std::vector<std::tuple<TfToken, DelegateCreator>> _delegates;
    std::vector<DelegatesChangedSignal>               _signals;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRALIB_DELEGATE_REGISTRY_H
