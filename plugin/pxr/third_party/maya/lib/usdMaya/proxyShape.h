//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXRUSDMAYA_PROXY_SHAPE_H
#define PXRUSDMAYA_PROXY_SHAPE_H

/// \file usdMaya/proxyShape.h

#include "usdMaya/api.h"
#include <mayaUsd/nodes/proxyShapeBase.h>

#include "pxr/pxr.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectionMask.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <map>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_PROXY_SHAPE_TOKENS \
    ((MayaTypeName, "pxrUsdProxyShape"))

TF_DECLARE_PUBLIC_TOKENS(UsdMayaProxyShapeTokens,
                         PXRUSDMAYA_API,
                         PXRUSDMAYA_PROXY_SHAPE_TOKENS);


class UsdMayaProxyShape : public MayaUsdProxyShapeBase
{
    public:
        typedef MayaUsdProxyShapeBase ParentClass;

        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        PXRUSDMAYA_API
        static MObject variantKeyAttr;
        PXRUSDMAYA_API
        static MObject fastPlaybackAttr;
        PXRUSDMAYA_API
        static MObject softSelectableAttr;

        /// Delegate function for returning whether object soft select mode is
        /// currently on
        typedef std::function<bool(void)> ObjectSoftSelectEnabledDelegate;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        PXRUSDMAYA_API
        static void SetObjectSoftSelectEnabledDelegate(
                ObjectSoftSelectEnabledDelegate delegate);

        // Virtual function overrides

        PXRUSDMAYA_API
        bool isBounded() const override;
        PXRUSDMAYA_API
        MBoundingBox boundingBox() const override;

        // Public functions
        PXRUSDMAYA_API
        bool setInternalValueInContext(
                const MPlug& plug,
                const MDataHandle& dataHandle,
                MDGContext& ctx) override;

        PXRUSDMAYA_API
        bool getInternalValueInContext(
                const MPlug& plug,
                MDataHandle& dataHandle,
                MDGContext& ctx) override;

    protected:
        // Use the variant key to get the session layer from the prim path.
        PXRUSDMAYA_API
        SdfLayerRefPtr computeSessionLayer(MDataBlock&) override;

        PXRUSDMAYA_API
        bool canBeSoftSelected() const override;

        PXRUSDMAYA_API
        bool GetObjectSoftSelectEnabled() const override;

    private:
        UsdMayaProxyShape();

        UsdMayaProxyShape(const UsdMayaProxyShape&);
        ~UsdMayaProxyShape() override;
        UsdMayaProxyShape& operator=(const UsdMayaProxyShape&);

        bool _useFastPlayback;

        static ObjectSoftSelectEnabledDelegate
            _sharedObjectSoftSelectEnabledDelegate;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
