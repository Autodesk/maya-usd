//
// Copyright 2024 Autodesk
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

#include "global.h"

#include <usdUfe/ufe/Global.h>

#include <pxr/base/tf/diagnostic.h>

#include <string>

namespace MAYAUSDAPI_NS_DEF {

Ufe::Rtid initialize(const DCCMandatoryFunctions& dccMandatoryFuncs)
{
    try {
        UsdUfe::DCCFunctions dccFuncs;
        dccFuncs.stageAccessorFn = dccMandatoryFuncs.stageAccessorFn;
        dccFuncs.stagePathAccessorFn = dccMandatoryFuncs.stagePathAccessorFn;
        dccFuncs.ufePathToPrimFn = dccMandatoryFuncs.ufePathToPrimFn;
        dccFuncs.timeAccessorFn = dccMandatoryFuncs.timeAccessorFn;

        UsdUfe::Handlers empty;
        return UsdUfe::initialize(dccFuncs, empty);
    } catch (const std::exception& e) {
        PXR_NAMESPACE_USING_DIRECTIVE
        std::string errMsg(e.what());
        TF_WARN(errMsg);
        return 0;
    }
}

bool finalize(bool exiting /*= false*/) { return UsdUfe::finalize(exiting); }

PXR_NS::TfNotice::Key registerStage(const PXR_NS::UsdStageRefPtr& stage)
{
    return UsdUfe::registerStage(stage);
}

bool revokeStage(PXR_NS::TfNotice::Key& key) { return PXR_NS::TfNotice::Revoke(key); }

} // namespace MAYAUSDAPI_NS_DEF
