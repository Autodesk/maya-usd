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

#include <mayaUsdAPI/proxyShapeNotice.h>

#include <pxr/base/tf/instantiateSingleton.h>
#include <pxr/base/tf/instantiateType.h>

#include <boost/python/def.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// Listen to mayaUsdAPI stage notices
class StageNoticesListener : public TfWeakBase
{
public:
    StageNoticesListener()
    {
        TfSingleton<StageNoticesListener>::SetInstanceConstructed(*this);
        TfWeakPtr<StageNoticesListener> ptr(this);
        _stageSetKey = TfNotice::Register(ptr, &StageNoticesListener::_StageSet);
        _stageInvalidateKey = TfNotice::Register(ptr, &StageNoticesListener::_StageInvalidate);
        _stageObjectsChangedKey
            = TfNotice::Register(ptr, &StageNoticesListener::_StageObjectsChanged);
        TF_VERIFY(
            _stageSetKey.IsValid() && _stageInvalidateKey.IsValid()
            && _stageObjectsChangedKey.IsValid());
    }

    ~StageNoticesListener() { _Revoke(); }

    // StageSet notice
    bool StageSetHasBeenCalled() const { return _stageSetCalled; }
    void ResetStageSet() { _stageSetCalled = false; }

    // StageInvalidate notice
    bool StageInvalidateHasBeenCalled() const { return _stageInvalidateCalled; }
    void ResetStageInvalidate() { _stageInvalidateCalled = false; }

    // StageObjectsChanged notice
    bool StageObjectsChangedHasBeenCalled() const { return _stageObjectsChangedCalled; }
    void ResetStageObjectsChanged() { _stageObjectsChangedCalled = false; }

private:
    void _StageSet(const MayaUsdAPI::ProxyStageSetNotice& notice) { _stageSetCalled = true; }
    void _StageInvalidate(const MayaUsdAPI::ProxyStageInvalidateNotice& notice)
    {
        _stageInvalidateCalled = true;
    }
    void _StageObjectsChanged(const MayaUsdAPI::ProxyStageObjectsChangedNotice& notice)
    {
        _stageObjectsChangedCalled = true;
    }

    void _Revoke()
    {
        if (_stageSetKey.IsValid()) {
            TfNotice::Revoke(_stageSetKey);
        }
        if (_stageInvalidateKey.IsValid()) {
            TfNotice::Revoke(_stageInvalidateKey);
        }
        if (_stageObjectsChangedKey.IsValid()) {
            TfNotice::Revoke(_stageObjectsChangedKey);
        }
    }

    TfNotice::Key _stageSetKey;
    static bool   _stageSetCalled;

    TfNotice::Key _stageInvalidateKey;
    static bool   _stageInvalidateCalled;

    TfNotice::Key _stageObjectsChangedKey;
    static bool   _stageObjectsChangedCalled;
};

bool StageNoticesListener::_stageSetCalled = false;
bool StageNoticesListener::_stageInvalidateCalled = false;
bool StageNoticesListener::_stageObjectsChangedCalled = false;

TF_INSTANTIATE_SINGLETON(StageNoticesListener);

// Create the singleton that starts listening to proxy shape notices
StageNoticesListener _instance = TfSingleton<StageNoticesListener>::GetInstance();

void ResetStageSetCall() { _instance.ResetStageSet(); }
bool StageSetHasBeenCalled() { return _instance.StageSetHasBeenCalled(); }

void ResetStageInvalidateCall() { _instance.ResetStageInvalidate(); }
bool StageInvalidateHasBeenCalled() { return _instance.StageInvalidateHasBeenCalled(); }

void ResetStageObjectsChangedCall() { _instance.ResetStageObjectsChanged(); }
bool StageObjectsChangedHasBeenCalled() { return _instance.StageObjectsChangedHasBeenCalled(); }

PXR_NAMESPACE_CLOSE_SCOPE

using namespace boost::python;
void wrapProxyShapeNotice()
{
    def("ResetStageSetCall", PXR_NS::ResetStageSetCall);
    def("StageSetHasBeenCalled", PXR_NS::StageSetHasBeenCalled);

    def("ResetStageInvalidateCall", PXR_NS::ResetStageInvalidateCall);
    def("StageInvalidateHasBeenCalled", PXR_NS::StageInvalidateHasBeenCalled);

    def("ResetStageObjectsChangedCall", PXR_NS::ResetStageObjectsChangedCall);
    def("StageObjectsChangedHasBeenCalled", PXR_NS::StageObjectsChangedHasBeenCalled);
}
