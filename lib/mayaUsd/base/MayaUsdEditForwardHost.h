//
// Copyright 2026 Autodesk
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
#ifndef MAYAUSD_EDITFORWARDHOST_H
#define MAYAUSD_EDITFORWARDHOST_H

#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>

#include <AdskUsdEditForward/Host.h>

/// \class MayaUsdEditForwardHost
/// \brief Maya-specific implementation of the USD Edit Forward host interface.
class MAYAUSD_CORE_PUBLIC MayaUsdEditForwardHost : public AdskUsdEditForward::Host
{
public:
    MayaUsdEditForwardHost() = default;
    ~MayaUsdEditForwardHost() = default;

    void ExecuteInCmd(std::function<void()> callback, bool immediate) override;

    bool IsEditForwardingPaused() const override;
    void PauseEditForwarding(bool pause) override;
    void TrackLayerStates(const pxr::SdfLayerHandle& layer) override;

private:
    bool _paused = false;
};

#endif // MAYAUSD_EDITFORWARDHOST_H
