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
#include <hdmaya/delegates/testDelegate.h>

#include <pxr/base/tf/envSetting.h>

#include <hdmaya/delegates/delegateRegistry.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HDMAYA_TEST_DELEGATE_FILE, "", "Path for HdMayaTestDelegate to load");

TF_DEFINE_PRIVATE_TOKENS(_tokens, (HdMayaTestDelegate));

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaDelegateRegistry, HdMayaTestDelegate) {
    if (!TfGetEnvSetting(HDMAYA_TEST_DELEGATE_FILE).empty()) {
        HdMayaDelegateRegistry::RegisterDelegate(
            _tokens->HdMayaTestDelegate,
            [](HdRenderIndex* parentIndex, const SdfPath& id) -> HdMayaDelegatePtr {
                return std::static_pointer_cast<HdMayaDelegate>(
                    std::make_shared<HdMayaTestDelegate>(parentIndex, id));
            });
    }
}

HdMayaTestDelegate::HdMayaTestDelegate(HdRenderIndex* renderIndex, const SdfPath& delegateID) {
    _delegate.reset(new UsdImagingDelegate(renderIndex, delegateID));
}

void HdMayaTestDelegate::Populate() {
    _stage = UsdStage::Open(TfGetEnvSetting(HDMAYA_TEST_DELEGATE_FILE));
    _delegate->Populate(_stage->GetPseudoRoot());
}

PXR_NAMESPACE_CLOSE_SCOPE
