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
#ifndef HDMAYA_TEST_DELEGATE_H
#define HDMAYA_TEST_DELEGATE_H

#include <hdMaya/delegates/delegate.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaTestDelegate : public HdMayaDelegate
{
public:
    HdMayaTestDelegate(const InitData& initData);

    void Populate() override;

private:
    std::unique_ptr<UsdImagingDelegate> _delegate;
    UsdStageRefPtr                      _stage;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_TEST_DELEGATE_H
