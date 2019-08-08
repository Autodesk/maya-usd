//
// Copyright 2019 Luma Pictures
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
#ifndef __MTOH_DEFAULT_LIGHT_DELEGATE_H__
#define __MTOH_DEFAULT_LIGHT_DELEGATE_H__

#include <pxr/pxr.h>

#include <pxr/imaging/glf/simpleLight.h>
#include <pxr/imaging/hd/renderIndex.h>

#include <pxr/usd/sdf/path.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <memory>

#include "../../usd/hdmaya/delegates/delegateCtx.h"

PXR_NAMESPACE_OPEN_SCOPE

class MtohDefaultLightDelegate : public HdSceneDelegate, public HdMayaDelegate {
public:
    MtohDefaultLightDelegate(const InitData& initData);

    ~MtohDefaultLightDelegate() override;

    void Populate() override;
    void SetDefaultLight(const GlfSimpleLight& light);

protected:
    GfMatrix4d GetTransform(const SdfPath& id) override;
    VtValue Get(const SdfPath& id, const TfToken& key) override;
    VtValue GetLightParamValue(
        const SdfPath& id, const TfToken& paramName) override;
    bool GetVisible(const SdfPath& id) override;

private:
    GlfSimpleLight _light;
    SdfPath _lightPath;
    bool _isSupported;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __MTOH_DEFAULT_LIGHT_DELEGATE_H__
