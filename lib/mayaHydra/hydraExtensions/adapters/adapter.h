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
#ifndef HDMAYA_ADAPTER_H
#define HDMAYA_ADAPTER_H

#include <hdMaya/api.h>
#include <hdMaya/delegates/delegateCtx.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MMessage.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaAdapter
{
public:
    HDMAYA_API
    HdMayaAdapter(const MObject& node, const SdfPath& id, HdMayaDelegateCtx* delegate);
    HDMAYA_API
    virtual ~HdMayaAdapter();

    const SdfPath&     GetID() const { return _id; }
    HdMayaDelegateCtx* GetDelegate() const { return _delegate; }
    HDMAYA_API
    void AddCallback(MCallbackId callbackId);
    HDMAYA_API
    virtual void RemoveCallbacks();
    HDMAYA_API
    virtual VtValue Get(const TfToken& key);
    const MObject&  GetNode() const { return _node; }
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
    SdfPath                  _id;
    std::vector<MCallbackId> _callbacks;
    HdMayaDelegateCtx*       _delegate;
    MObject                  _node;

    bool _isPopulated = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_ADAPTER_H
