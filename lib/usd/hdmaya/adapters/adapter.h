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
#ifndef __HDMAYA_ADAPTER_H__
#define __HDMAYA_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/usd/sdf/path.h>

#include <maya/MMessage.h>

#include <vector>

#include "../api.h"
#include "../delegates/delegateCtx.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapter {
public:
    HDMAYA_API
    HdMayaAdapter(
        const MObject& node, const SdfPath& id, HdMayaDelegateCtx* delegate);
    HDMAYA_API
    virtual ~HdMayaAdapter();

    const SdfPath& GetID() const { return _id; }
    HdMayaDelegateCtx* GetDelegate() const { return _delegate; }
    HDMAYA_API
    void AddCallback(MCallbackId callbackId);
    HDMAYA_API
    virtual void RemoveCallbacks();
    HDMAYA_API
    virtual VtValue Get(const TfToken& key);
    const MObject& GetNode() const { return _node; }
    HDMAYA_API
    virtual bool IsSupported() const = 0;
    HDMAYA_API
    virtual bool HasType(const TfToken& typeId) const;

    HDMAYA_API
    virtual void CreateCallbacks();
    virtual void MarkDirty(HdDirtyBits dirtyBits) = 0;
    virtual void RemovePrim() = 0;
    virtual void Populate() = 0;

    HDMAYA_API
    static MStatus Initialize();

    bool IsPopulated() const { return _isPopulated; }

protected:
    SdfPath _id;
    std::vector<MCallbackId> _callbacks;
    HdMayaDelegateCtx* _delegate;
    MObject _node;

    bool _isPopulated = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_ADAPTER_H__
