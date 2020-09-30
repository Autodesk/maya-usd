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
#ifndef HDMAYA_AL_PROXY_DELEGATE_H
#define HDMAYA_AL_PROXY_DELEGATE_H

#include <memory>

#include <maya/MMessage.h>

#include <pxr/pxr.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usdImaging/usdImaging/delegate.h>

#include <hdMaya/delegates/delegate.h>

#if WANT_UFE_BUILD
#include <ufe/selection.h>
#endif // WANT_UFE_BUILD

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

    // TODO: implement this override this to add selection support
    // for non-ufe
//    void PopulateSelectedPaths(
//        const MSelectionList& mayaSelection, SdfPathVector& selectedSdfPaths,
//        const HdSelectionSharedPtr& selection) override;

#if WANT_UFE_BUILD
    void PopulateSelectedPaths(
        const UFE_NS::Selection& ufeSelection, SdfPathVector& selectedSdfPaths,
        const HdSelectionSharedPtr& selection) override;
    bool SupportsUfeSelection() override;
#endif // WANT_UFE_BUILD

#if MAYA_API_VERSION >= 20210000
    void PopulateSelectionList(
        const HdxPickHitVector& hits,
        const MHWRender::MSelectionInfo& selectInfo,
        MSelectionList& selectionList,
        MPointArray& worldSpaceHitPts) override;
#endif
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_AL_PROXY_DELEGATE_H
