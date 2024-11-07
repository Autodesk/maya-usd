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
#include <pxr_python.h>

namespace {

// Listen to mayaUsdAPI stage notices
class StageNoticesListener : public PXR_NS::TfWeakBase
{
public:
    StageNoticesListener()
    {
        PXR_NAMESPACE_USING_DIRECTIVE
        TfSingleton<StageNoticesListener>::SetInstanceConstructed(*this);
        TfWeakPtr<StageNoticesListener> ptr(this);
        _stageSetKey = TfNotice::Register(ptr, &StageNoticesListener::stageSet_);
        _stageInvalidatedKey = TfNotice::Register(ptr, &StageNoticesListener::stageInvalidate_);
        _stageObjectsChangedKey
            = TfNotice::Register(ptr, &StageNoticesListener::stageObjectsChanged_);
        TF_VERIFY(
            _stageSetKey.IsValid() && _stageInvalidatedKey.IsValid()
            && _stageObjectsChangedKey.IsValid());
    }

    ~StageNoticesListener() { revoke_(); }

    // StageSet notice
    bool stageSet() const { return _stageSet; }
    void resetStageSet() { _stageSet = false; }

    // StageInvalidated notice
    bool stageInvalidated() const { return _stageInvalidated; }
    void resetStageInvalidated() { _stageInvalidated = false; }

    // StageObjectsChanged notice
    bool stageObjectsChanged() const { return _stageObjectsChanged; }
    void resetStageObjectsChanged() { _stageObjectsChanged = false; }

private:
    void stageSet_(const MayaUsdAPI::ProxyStageSetNotice& notice) { _stageSet = true; }
    void stageInvalidate_(const MayaUsdAPI::ProxyStageInvalidateNotice& notice)
    {
        _stageInvalidated = true;
    }
    void stageObjectsChanged_(const MayaUsdAPI::ProxyStageObjectsChangedNotice& notice)
    {
        _stageObjectsChanged = true;
    }

    void revoke_()
    {
        if (_stageSetKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_stageSetKey);
        }
        if (_stageInvalidatedKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_stageInvalidatedKey);
        }
        if (_stageObjectsChangedKey.IsValid()) {
            PXR_NS::TfNotice::Revoke(_stageObjectsChangedKey);
        }
    }

    PXR_NS::TfNotice::Key _stageSetKey;
    static bool           _stageSet;

    PXR_NS::TfNotice::Key _stageInvalidatedKey;
    static bool           _stageInvalidated;

    PXR_NS::TfNotice::Key _stageObjectsChangedKey;
    static bool           _stageObjectsChanged;
};

bool StageNoticesListener::_stageSet = false;
bool StageNoticesListener::_stageInvalidated = false;
bool StageNoticesListener::_stageObjectsChanged = false;

// Create the singleton that starts listening to proxy shape notices
StageNoticesListener _instance = PXR_NS::TfSingleton<StageNoticesListener>::GetInstance();

void resetStageSet() { _instance.resetStageSet(); }
bool stageSet() { return _instance.stageSet(); }

void resetStageInvalidated() { _instance.resetStageInvalidated(); }
bool stageInvalidated() { return _instance.stageInvalidated(); }

void resetStageObjectsChanged() { _instance.resetStageObjectsChanged(); }
bool stageObjectsChanged() { return _instance.stageObjectsChanged(); }
} // namespace

PXR_NAMESPACE_OPEN_SCOPE
TF_INSTANTIATE_SINGLETON(StageNoticesListener); // Cannot be in anonymous namespace
PXR_NAMESPACE_CLOSE_SCOPE

using namespace PXR_BOOST_PYTHON_NAMESPACE;
void wrapProxyShapeNotice()
{
    def("resetStageSet", resetStageSet);
    def("stageSet", stageSet);

    def("resetStageInvalidated", resetStageInvalidated);
    def("stageInvalidated", stageInvalidated);

    def("resetStageObjectsChanged", resetStageObjectsChanged);
    def("stageObjectsChanged", stageObjectsChanged);
}
