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
#ifndef __HDMAYA_DG_ADAPTER_H__
#define __HDMAYA_DG_ADAPTER_H__

#include <pxr/pxr.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/range3d.h>

#include <pxr/base/tf/token.h>

#include <pxr/imaging/hd/meshTopology.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/imaging/hd/sceneDelegate.h>

#include <functional>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MFnDagNode.h>
#include <maya/MMatrix.h>

#include <maya/MMessage.h>

#include <hdmaya/adapters/adapter.h>
#include <hdmaya/adapters/adapterDebugCodes.h>
#include <hdmaya/utils.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaDagAdapter : public HdMayaAdapter {
protected:
    HDMAYA_API
    HdMayaDagAdapter(
        const SdfPath& id, HdMayaDelegateCtx* delegate,
        const MDagPath& dagPath);

public:
    HDMAYA_API
    virtual ~HdMayaDagAdapter() = default;

    HDMAYA_API
    virtual bool GetVisible();
    HDMAYA_API
    virtual void CreateCallbacks() override;
    HDMAYA_API
    virtual void MarkDirty(HdDirtyBits dirtyBits) override;
    HDMAYA_API
    virtual void RemovePrim() override;
    HDMAYA_API
    virtual void PopulateSelection(
        const HdSelection::HighlightMode& mode, HdSelection* selection);

    HDMAYA_API
    GfMatrix4d& GetTransform();
    const MDagPath& GetDagPath() { return _dagPath; }

protected:
    HDMAYA_API
    void CalculateTransform();

private:
    MDagPath _dagPath;
    GfMatrix4d _transform;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __HDMAYA_DG_ADAPTER_H__
