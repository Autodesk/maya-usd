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
#ifndef __HDMAYA_ADAPTER_H__
#define __HDMAYA_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MMessage.h>

#include <vector>

#include <hdmaya/api.h>
#include <hdmaya/delegates/delegateCtx.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapter {
public:
    HDMAYA_API
    HdMayaAdapter(
        const MObject& node, const SdfPath& id, HdMayaDelegateCtx* delegate);
    HDMAYA_API
    virtual ~HdMayaAdapter();

    const SdfPath& GetID() { return _id; }
    HdMayaDelegateCtx* GetDelegate() { return _delegate; }
    HDMAYA_API
    void AddCallback(MCallbackId callbackId);
    HDMAYA_API
    void RemoveCallbacks();
    HDMAYA_API
    virtual VtValue Get(const TfToken& key);
    const MObject& GetNode() { return _node; }
    HDMAYA_API
    virtual bool IsSupported() = 0;
    HDMAYA_API
    virtual bool HasType(const TfToken& typeId);

    HDMAYA_API
    virtual void CreateCallbacks();
    virtual void MarkDirty(HdDirtyBits dirtyBits) = 0;
    virtual void RemovePrim() = 0;
    virtual void Populate() = 0;

    HDMAYA_API
    static MStatus Initialize();

protected:
    MObject _node;
    SdfPath _id;
    HdMayaDelegateCtx* _delegate;

    std::vector<MCallbackId> _callbacks;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_H__
