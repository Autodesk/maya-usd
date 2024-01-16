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

#include <assert.h>

namespace {

// Listen to mayaUsdAPI stage notices
class StageNoticesListener : public PXR_NS::TfWeakBase
{
public:
    StageNoticesListener()
    {
        PXR_NS::TfSingleton<StageNoticesListener>::SetInstanceConstructed(*this);
        PXR_NS::TfWeakPtr<StageNoticesListener> ptr(this);
        _stageSetKey = PXR_NS::TfNotice::Register(ptr, &StageNoticesListener::_StageSet);
        _stageInvalidateKey
            = PXR_NS::TfNotice::Register(ptr, &StageNoticesListener::_StageInvalidate);
        _stageObjectsChangedKey
            = PXR_NS::TfNotice::Register(ptr, &StageNoticesListener::_StageObjectsChanged);
        assert(
            _stageSetKey.IsValid() && _stageInvalidateKey.IsValid()
            && _stageObjectsChangedKey.IsValid());
    }

    ~StageNoticesListener() { _Revoke(); }

    // StageSet notice
    bool StageSet() const { return _bStageSet; }
    void ResetStageSet() { _bStageSet = false; }

    // StageInvalidate notice
    bool StageInvalidate() const { return _bStageInvalidate; }
    void ResetStageInvalidate() { _bStageInvalidate = false; }

    // StageObjectsChanged notice
    bool StageObjectsChanged() const { return _bStageObjectsChanged; }
    void ResetStageObjectsChanged() { _bStageObjectsChanged = false; }

private:
    void _StageSet(const MayaUsdAPI::ProxyStageSetNotice& notice) { _bStageSet = true; }
    void _StageInvalidate(const MayaUsdAPI::ProxyStageInvalidateNotice& notice)
    {
        _bStageInvalidate = true;
    }
    void _StageObjectsChanged(const MayaUsdAPI::ProxyStageObjectsChangedNotice& notice)
    {
        _bStageObjectsChanged = true;
    }

    void _Revoke()
    {
        if (_stageSetKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_stageSetKey);
        }
        if (_stageInvalidateKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_stageInvalidateKey);
        }
        if (_stageObjectsChangedKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_stageObjectsChangedKey);
        }
    }

    PXR_NS::TfNotice::Key _stageSetKey;
    static bool           _bStageSet;

    PXR_NS::TfNotice::Key _stageInvalidateKey;
    static bool           _bStageInvalidate;

    PXR_NS::TfNotice::Key _stageObjectsChangedKey;
    static bool           _bStageObjectsChanged;
};

bool StageNoticesListener::_bStageSet = false;
bool StageNoticesListener::_bStageInvalidate = false;
bool StageNoticesListener::_bStageObjectsChanged = false;

PXR_NAMESPACE_OPEN_SCOPE
TF_INSTANTIATE_SINGLETON(StageNoticesListener);
PXR_NAMESPACE_CLOSE_SCOPE
// Create the singleton that starts listening to proxy shape notices
StageNoticesListener _instance = PXR_NS::TfSingleton<StageNoticesListener>::GetInstance();

void ResetStageSet() { _instance.ResetStageSet(); }
bool StageSet() { return _instance.StageSet(); }

void ResetStageInvalidate() { _instance.ResetStageInvalidate(); }
bool StageInvalidate() { return _instance.StageInvalidate(); }

void ResetStageObjectsChanged() { _instance.ResetStageObjectsChanged(); }
bool StageObjectsChanged() { return _instance.StageObjectsChanged(); }
} // namespace

using namespace boost::python;
void wrapProxyShapeNotice()
{
    def("ResetStageSet", ResetStageSet);
    def("StageSet", StageSet);

    def("ResetStageInvalidate", ResetStageInvalidate);
    def("StageInvalidate", StageInvalidate);

    def("ResetStageObjectsChanged", ResetStageObjectsChanged);
    def("StageObjectsChanged", StageObjectsChanged);
}
