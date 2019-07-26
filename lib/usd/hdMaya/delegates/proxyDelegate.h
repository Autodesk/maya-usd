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

#ifndef __HDMAYA_AL_PROXY_DELEGATE_H__
#define __HDMAYA_AL_PROXY_DELEGATE_H__

#include "delegate.h"

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MMessage.h>

#include <memory>

#if HDMAYA_UFE_BUILD
#include <ufe/selection.h>
#endif // HDMAYA_UFE_BUILD

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaProxyAdapter;
class MayaUsdProxyShapeBase;

class HdMayaProxyDelegate : public HdMayaDelegate {
public:
    HdMayaProxyDelegate(const InitData& initData);

    ~HdMayaProxyDelegate() override;

    static HdMayaDelegatePtr Creator(const InitData& initData);

    static void AddAdapter(HdMayaProxyAdapter* adapter);
    static void RemoveAdapter(HdMayaProxyAdapter* adapter);

    void Populate() override;
    void PreFrame(const MHWRender::MDrawContext& context) override;
    void PopulateSelectedPaths(
        const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection) override;

#if HDMAYA_UFE_BUILD
    void PopulateSelectedPaths(
        const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection) override;
    bool SupportsUfeSelection() override;
#endif // HDMAYA_UFE_BUILD
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_AL_PROXY_DELEGATE_H__
