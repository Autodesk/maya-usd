//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_HD_IMAGING_SHAPE_H
#define PXRUSDMAYA_HD_IMAGING_SHAPE_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/staticTokens.h>
#include <pxr/pxr.h>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MMessage.h>
#include <maya/MNodeMessage.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

PXR_NAMESPACE_OPEN_SCOPE

#define PXRUSDMAYA_HD_IMAGING_SHAPE_TOKENS ((MayaTypeName, "pxrHdImagingShape"))

TF_DECLARE_PUBLIC_TOKENS(
    PxrMayaHdImagingShapeTokens,
    MAYAUSD_CORE_PUBLIC,
    PXRUSDMAYA_HD_IMAGING_SHAPE_TOKENS);

/// Simple Maya shape providing batched drawing of other shapes imaged by Hydra.
///
/// This shape does nothing other than to act as a single invocation point for
/// Hydra task execution for all other shapes in the scene that are imaged by
/// Hydra. Those other shapes will respond to Maya's requests for draw
/// preparation, but the actual drawing of those shapes by Hydra will only be
/// invoked when this shape is drawn.
class PxrMayaHdImagingShape : public MPxSurfaceShape
{
public:
    MAYAUSD_CORE_PUBLIC
    static const MTypeId typeId;
    MAYAUSD_CORE_PUBLIC
    static const MString typeName;

    // Attributes
    MAYAUSD_CORE_PUBLIC
    static MObject selectionResolutionAttr;
    MAYAUSD_CORE_PUBLIC
    static MObject enableDepthSelectionAttr;

    MAYAUSD_CORE_PUBLIC
    static void* creator();

    MAYAUSD_CORE_PUBLIC
    static MStatus initialize();

    MAYAUSD_CORE_PUBLIC
    static PxrMayaHdImagingShape* GetShapeAtDagPath(const MDagPath& dagPath);

    /// Gets the "singleton" instance of the shape if it exists, or creates
    /// it if it doesn't.
    ///
    /// There is typically only one instance of this node in a Maya scene
    /// that takes care of all Hydra imaging for the scene. This method can
    /// be used to ensure that that instance exists, and to get the MObject
    /// for it.
    /// Note that since this node is a shape, it is required to have a
    /// transform node as a parent. This method will create that node as
    /// well and set it up such that it will *not* save into the Maya scene
    /// file. The nodes are also locked to prevent accidental deletion,
    /// re-naming, or re-parenting.
    MAYAUSD_CORE_PUBLIC
    static MObject GetOrCreateInstance();

    // MPxSurfaceShape Overrides

    MAYAUSD_CORE_PUBLIC
    bool isBounded() const override;

    MAYAUSD_CORE_PUBLIC
    MBoundingBox boundingBox() const override;

    // MPxNode Overrides

    MAYAUSD_CORE_PUBLIC
    void postConstructor() override;

    MAYAUSD_CORE_PUBLIC
    bool getInternalValue(const MPlug& plug, MDataHandle& dataHandle) override;

    MAYAUSD_CORE_PUBLIC
    bool setInternalValue(const MPlug& plug, const MDataHandle& dataHandle) override;

private:
    // The callback IDs and functions below are used to ensure that this
    // shape is always inserted into any viewport isolate selection set.
    MCallbackId                                         _objectSetAddedCallbackId;
    MCallbackId                                         _objectSetRemovedCallbackId;
    UsdMayaUtil::MObjectHandleUnorderedMap<MCallbackId> _objectSetAttrChangedCallbackIds;

    static void _OnObjectSetAdded(MObject& node, void* clientData);
    static void _OnObjectSetRemoved(MObject& node, void* clientData);
    static void _OnObjectSetAttrChanged(
        MNodeMessage::AttributeMessage msg,
        MPlug&                         plug,
        MPlug&                         otherPlug,
        void*                          clientData);

    PxrMayaHdImagingShape();
    ~PxrMayaHdImagingShape() override;

    PxrMayaHdImagingShape(const PxrMayaHdImagingShape&) = delete;
    PxrMayaHdImagingShape& operator=(const PxrMayaHdImagingShape&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
