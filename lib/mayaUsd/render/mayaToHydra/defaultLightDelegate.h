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
#ifndef MTOH_DEFAULT_LIGHT_DELEGATE_H
#define MTOH_DEFAULT_LIGHT_DELEGATE_H

#include <hdMaya/delegates/delegateCtx.h>

#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

PXR_NAMESPACE_OPEN_SCOPE

class MtohDefaultLightDelegate
    : public HdSceneDelegate
    , public HdMayaDelegate
{
public:
    MtohDefaultLightDelegate(const InitData& initData);

    ~MtohDefaultLightDelegate() override;

    void Populate() override;
    void SetDefaultLight(const GlfSimpleLight& light);

protected:
    GfMatrix4d GetTransform(const SdfPath& id) override;
    VtValue    Get(const SdfPath& id, const TfToken& key) override;
    VtValue    GetLightParamValue(const SdfPath& id, const TfToken& paramName) override;
    bool       GetVisible(const SdfPath& id) override;

private:
    GlfSimpleLight _light;
    SdfPath        _lightPath;
    bool           _isSupported = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MTOH_DEFAULT_LIGHT_DELEGATE_H
