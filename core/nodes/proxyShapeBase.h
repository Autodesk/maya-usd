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
#ifndef PXRUSDMAYA_PROXY_SHAPE_BASE_H
#define PXRUSDMAYA_PROXY_SHAPE_BASE_H

/// \file usdMaya/proxyShapeBase.h

#include "../base/api.h"
#include "usdMaya/stageNoticeListener.h"
#include "usdMaya/usdPrimProvider.h"

#include "pxr/pxr.h"

#include "pxr/base/gf/ray.h"
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


#define MAYAUSD_PROXY_SHAPE_BASE_TOKENS \
    ((MayaTypeName, "mayaUsdProxyShapeBase"))

TF_DECLARE_PUBLIC_TOKENS(MayaUsdProxyShapeBaseTokens,
                         PXRUSDMAYA_API,
                         MAYAUSD_PROXY_SHAPE_BASE_TOKENS);


class MayaUsdProxyShapeBase : public MPxSurfaceShape,
                              public UsdMayaUsdPrimProvider
{
    public:
        typedef MayaUsdProxyShapeBase ThisClass;

        PXRUSDMAYA_API
        static const MTypeId typeId;
        PXRUSDMAYA_API
        static const MString typeName;

        PXRUSDMAYA_API
        static const MString displayFilterName;
        PXRUSDMAYA_API
        static const MString displayFilterLabel;

        // Attributes
        PXRUSDMAYA_API
        static MObject filePathAttr;
        PXRUSDMAYA_API
        static MObject primPathAttr;
        PXRUSDMAYA_API
        static MObject excludePrimPathsAttr;
        PXRUSDMAYA_API
        static MObject timeAttr;
        PXRUSDMAYA_API
        static MObject complexityAttr;
        PXRUSDMAYA_API
        static MObject inStageDataAttr;
        PXRUSDMAYA_API
        static MObject inStageDataCachedAttr;
        PXRUSDMAYA_API
        static MObject outStageDataAttr;
        PXRUSDMAYA_API
        static MObject drawRenderPurposeAttr;
        PXRUSDMAYA_API
        static MObject drawProxyPurposeAttr;
        PXRUSDMAYA_API
        static MObject drawGuidePurposeAttr;

        /// Delegate function for computing the closest point and surface normal
        /// on the proxy shape to a given ray.
        /// The input ray, output point, and output normal should be in the
        /// proxy shape's local space.
        /// Should return true if a point was found, and false otherwise.
        /// (You could just treat this as a ray intersection and return true
        /// if intersected, false if missed.)
        typedef std::function<bool(const MayaUsdProxyShapeBase&, const GfRay&,
                GfVec3d*, GfVec3d*)> ClosestPointDelegate;

        PXRUSDMAYA_API
        static void* creator();

        PXRUSDMAYA_API
        static MStatus initialize();

        PXRUSDMAYA_API
        static MayaUsdProxyShapeBase* GetShapeAtDagPath(const MDagPath& dagPath);

        PXRUSDMAYA_API
        static void SetClosestPointDelegate(ClosestPointDelegate delegate);

        // UsdMayaUsdPrimProvider overrides:
        /**
         * accessor to get the usdprim
         *
         * This method pulls the usdstage data from outData, and will evaluate
         * the dependencies necessary to do so. It should be called instead of
         * pulling on the data directly.
         */
        PXRUSDMAYA_API
        UsdPrim usdPrim() const override;

        // Virtual function overrides

        // A ProxyShapeBase node cannot be created directly; it only exists
        // as a base class.
        PXRUSDMAYA_API
        bool isAbstractClass() const override;
        PXRUSDMAYA_API
        void postConstructor() override;
        PXRUSDMAYA_API
        MStatus compute(
                const MPlug& plug,
                MDataBlock& dataBlock) override;
        PXRUSDMAYA_API
        bool isBounded() const override;
        PXRUSDMAYA_API
        MBoundingBox boundingBox() const override;
        PXRUSDMAYA_API
        MSelectionMask getShapeSelectionMask() const override;

        PXRUSDMAYA_API
        bool closestPoint(
                const MPoint& raySource,
                const MVector& rayDirection,
                MPoint& theClosestPoint,
                MVector& theClosestNormal,
                bool findClosestOnMiss,
                double tolerance) override;

        PXRUSDMAYA_API
        bool canMakeLive() const override;

        // Public functions
        PXRUSDMAYA_API
        virtual SdfPathVector getExcludePrimPaths() const;

        PXRUSDMAYA_API
        int getComplexity() const;
        PXRUSDMAYA_API
        UsdTimeCode getTime() const;

        PXRUSDMAYA_API
        bool GetAllRenderAttributes(
                UsdPrim* usdPrimOut,
                SdfPathVector* excludePrimPathsOut,
                int* complexityOut,
                UsdTimeCode* timeOut,
                bool* drawRenderPurpose,
                bool* drawProxyPurpose,
                bool* drawGuidePurpose);

        PXRUSDMAYA_API
        MStatus setDependentsDirty(
                const MPlug& plug,
                MPlugArray& plugArray) override;

        /// \brief  Clears the bounding box cache of the shape
        PXRUSDMAYA_API
        void clearBoundingBoxCache();

    protected:
        PXRUSDMAYA_API
        MayaUsdProxyShapeBase();

        PXRUSDMAYA_API
        ~MayaUsdProxyShapeBase() override;

        PXRUSDMAYA_API
        bool isStageValid() const;

        // Hook method for derived classes.  This class returns a nullptr.
        PXRUSDMAYA_API
        virtual SdfLayerRefPtr computeSessionLayer(MDataBlock&);

        // Hook method for derived classes: can this object be soft selected?
        // This class returns false.
        PXRUSDMAYA_API
        virtual bool canBeSoftSelected() const;

        // Hook method for derived classes: is soft select enabled?  This
        // class returns false.
        PXRUSDMAYA_API
        virtual bool GetObjectSoftSelectEnabled() const;

        PXRUSDMAYA_API
        UsdPrim _GetUsdPrim(MDataBlock dataBlock) const;

        // Hook method for derived classes: cache an empty computed bounding
        // box.  This class does nothing.
        PXRUSDMAYA_API
        virtual void CacheEmptyBoundingBox(MBoundingBox&);

        // Return the output time.  This class returns the value of the 
        // input time attribute.
        PXRUSDMAYA_API
        virtual UsdTimeCode GetOutputTime(MDataBlock) const;

    private:
        MayaUsdProxyShapeBase(const MayaUsdProxyShapeBase&);
        MayaUsdProxyShapeBase& operator=(const MayaUsdProxyShapeBase&);

        MStatus computeInStageDataCached(MDataBlock& dataBlock);
        MStatus computeOutStageData(MDataBlock& dataBlock);

        SdfPathVector _GetExcludePrimPaths(MDataBlock dataBlock) const;
        int _GetComplexity(MDataBlock dataBlock) const;
        UsdTimeCode _GetTime(MDataBlock dataBlock) const;

        bool _GetDrawPurposeToggles(
                MDataBlock dataBlock,
                bool* drawRenderPurpose,
                bool* drawProxyPurpose,
                bool* drawGuidePurpose) const;

        void _OnStageContentsChanged(
                const UsdNotice::StageContentsChanged& notice);

        UsdMayaStageNoticeListener _stageNoticeListener;

        std::map<UsdTimeCode, MBoundingBox> _boundingBoxCache;

        static ClosestPointDelegate _sharedClosestPointDelegate;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
