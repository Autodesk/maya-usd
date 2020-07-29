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
#include "testDelegate.h"

#include <hdMaya/delegates/delegateRegistry.h>

#include <pxr/base/tf/envSetting.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDMAYA_TEST_DELEGATE_FILE, "", "Path for HdMayaTestDelegate to load");

TF_DEFINE_PRIVATE_TOKENS(_tokens, (HdMayaTestDelegate));

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaTestDelegate)
{
    if (!TfGetEnvSetting(HDMAYA_TEST_DELEGATE_FILE).empty()) {
        HdMayaDelegateRegistry::RegisterDelegate(
            _tokens->HdMayaTestDelegate,
            [](const HdMayaDelegate::InitData& initData) -> HdMayaDelegatePtr {
                return std::static_pointer_cast<HdMayaDelegate>(
                    std::make_shared<HdMayaTestDelegate>(initData));
            });
    }
}

HdMayaTestDelegate::HdMayaTestDelegate(const InitData& initData)
    : HdMayaDelegate(initData)
{
    _delegate.reset(new UsdImagingDelegate(initData.renderIndex, initData.delegateID));
}

void HdMayaTestDelegate::Populate()
{
    _stage = UsdStage::Open(TfGetEnvSetting(HDMAYA_TEST_DELEGATE_FILE));
    _delegate->Populate(_stage->GetPseudoRoot());
}

PXR_NAMESPACE_CLOSE_SCOPE
