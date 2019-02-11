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

#include <hdmaya/delegates/delegate.h>

#include <AL/usdmaya/nodes/ProxyShape.h>

#include <pxr/pxr.h>

#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <maya/MMessage.h>

#include <memory>
#include <unordered_map>

#if HDMAYA_UFE_BUILD
#include <ufe/selection.h>
#endif // HDMAYA_UFE_BUILD

using AL::usdmaya::nodes::ProxyShape;

PXR_NAMESPACE_OPEN_SCOPE

struct HdMayaALProxyData {
    std::vector<AL::event::CallbackId> proxyShapeCallbacks;
    std::unique_ptr<UsdImagingDelegate> delegate;
    bool populated = false;
};

class HdMayaALProxyDelegate : public HdMayaDelegate {
public:
    HdMayaALProxyDelegate(
        HdRenderIndex* renderIndex, const SdfPath& delegateID);

    ~HdMayaALProxyDelegate() override;

    static HdMayaDelegatePtr Creator(
        HdRenderIndex* parentIndex, const SdfPath& id);

    void Populate() override;
    void PreFrame(const MHWRender::MDrawContext& context) override;
    void PopulateSelectedPaths(
        const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
        HdSelection* selection) override;

#if HDMAYA_UFE_BUILD
    void PopulateSelectedPaths(
        const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
        HdSelection* selection) override;
    bool SupportsUfeSelection() override;
#endif // HDMAYA_UFE_BUILD

    HdMayaALProxyData& AddProxy(ProxyShape* proxy);
    void RemoveProxy(ProxyShape* proxy);
    void CreateUsdImagingDelegate(ProxyShape* proxy);
    void DeleteUsdImagingDelegate(ProxyShape* proxy);

private:
    bool PopulateSingleProxy(ProxyShape* proxy, HdMayaALProxyData& proxyData);
    void CreateUsdImagingDelegate(
        ProxyShape* proxy, HdMayaALProxyData& proxyData);

    std::unordered_map<ProxyShape*, HdMayaALProxyData> _proxiesData;
    SdfPath const _delegateID;
    HdRenderIndex* _renderIndex;
    MCallbackId _nodeAddedCBId;
    MCallbackId _nodeRemovedCBId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_AL_PROXY_DELEGATE_H__
