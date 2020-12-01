//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_PRIM_WRITER_H
#define PXRUSDMAYA_PRIM_WRITER_H

#include <mayaUsd/base/api.h>
#include <mayaUsd/fileio/jobs/jobArgs.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaWriteJobContext;

/// Base class for all built-in and user-defined prim writers. Translates Maya
/// node data into USD prim(s).
///
/// Note that this class can be used to write USD prims for both DG and DAG
/// Maya nodes. For DAG nodes, an MDagPath is required to uniquely identify
/// instances in the DAG, so the writer should be created using an
/// MFnDagNode (or one of its derived classes) that was constructed using an
/// MDagPath, *not* an MObject.
class UsdMayaPrimWriter
{
public:
    /// Constructs a prim writer for writing a Maya DG or DAG node.
    ///
    /// Note that if the Maya node is a DAG node, this must be passed an
    /// MFnDagNode (or one of its derived classes) that was constructed with
    /// an MDagPath to ensure that instancing is handled correctly. An error
    /// will be issued if the constructor receives an MFnDagNode *not*
    /// constructed with an MDagPath.
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx);

    MAYAUSD_CORE_PUBLIC
    virtual ~UsdMayaPrimWriter();

    /// Main export function that runs when the traversal hits the node.
    /// The default implementation writes attributes for the UsdGeomImageable
    /// and UsdGeomGprim schemas if the prim conforms to one or both; in most
    /// cases, subclasses will want to invoke the base class Write() method
    /// when overriding.
    MAYAUSD_CORE_PUBLIC
    virtual void Write(const UsdTimeCode& usdTime);

    /// Post export function that runs before saving the stage.
    ///
    /// Base implementation does nothing.
    MAYAUSD_CORE_PUBLIC
    virtual void PostExport();

    /// Whether this prim writer directly create one or more gprims on the
    /// current model on the USD stage. (Excludes cases where the prim writer
    /// introduces gprims via a reference or by adding a sub-model, such as in
    /// a point instancer.)
    ///
    /// Base implementation returns \c false; prim writers exporting
    /// gprim (shape) classes should override.
    MAYAUSD_CORE_PUBLIC
    virtual bool ExportsGprims() const;

    /// Whether the traversal routine using this prim writer should skip all of
    /// the Maya node's descendants when continuing traversal.
    /// If you override this to return \c true, you may also want to override
    /// GetDagToUsdPathMapping() if you handle export of descendant nodes
    /// (though that is not required).
    ///
    /// Base implementation returns \c false; prim writers that handle export
    /// for their entire subtree should override.
    MAYAUSD_CORE_PUBLIC
    virtual bool ShouldPruneChildren() const;

    /// Whether visibility can be exported for this prim.
    /// By default, this is based off of the export visibility setting in the
    /// export args.
    MAYAUSD_CORE_PUBLIC
    bool GetExportVisibility() const;

    /// Sets whether visibility can be exported for this prim.
    /// This will override the export args.
    MAYAUSD_CORE_PUBLIC
    void SetExportVisibility(const bool exportVis);

    /// Gets all of the exported prim paths that are potentially models, i.e.
    /// the prims on which this prim writer has authored kind metadata or
    /// otherwise expects kind metadata to exist (e.g. via reference).
    ///
    /// The USD export process will attempt to "fix-up" kind metadata to ensure
    /// contiguous model hierarchy for any potential model prims.
    ///
    /// The base implementation returns an empty vector.
    MAYAUSD_CORE_PUBLIC
    virtual const SdfPathVector& GetModelPaths() const;

    /// Gets a mapping from MDagPaths to exported prim paths.
    /// Useful only for DAG prim writers that override ShouldPruneChildren() to
    /// \c true but still want the export process to know about the Maya-to-USD
    /// correspondence for their descendants, e.g., for material binding
    /// purposes.
    /// The result vector should only include paths for which there is a true,
    /// one-to-one correspondence between the Maya node and USD prim; don't
    /// include any mappings where the mapped value is an invalid path.
    ///
    /// The base implementation for DAG prim writers simply maps GetDagPath()
    /// to GetUsdPath(). For DG prim writers, an empty map is returned.
    MAYAUSD_CORE_PUBLIC
    virtual const UsdMayaUtil::MDagPathMap<SdfPath>& GetDagToUsdPathMapping() const;

    /// The source Maya DAG path that we are consuming.
    ///
    /// If this prim writer is for a Maya DG node and not a DAG node, this will
    /// return an invalid MDagPath.
    MAYAUSD_CORE_PUBLIC
    const MDagPath& GetDagPath() const;

    /// The MObject for the Maya node being written by this writer.
    MAYAUSD_CORE_PUBLIC
    const MObject& GetMayaObject() const;

    /// The path of the destination USD prim to which we are writing.
    MAYAUSD_CORE_PUBLIC
    const SdfPath& GetUsdPath() const;

    /// The destination USD prim to which we are writing.
    MAYAUSD_CORE_PUBLIC
    const UsdPrim& GetUsdPrim() const;

    /// Gets the USD stage that we're writing to.
    MAYAUSD_CORE_PUBLIC
    const UsdStageRefPtr& GetUsdStage() const;

protected:
    /// Helper function for determining whether the current node has input
    /// animation curves.
    MAYAUSD_CORE_PUBLIC
    virtual bool _HasAnimCurves() const;

    /// Gets the current global export args in effect.
    MAYAUSD_CORE_PUBLIC
    const UsdMayaJobExportArgs& _GetExportArgs() const;

    /// Get the attribute value-writer object to be used when writing
    /// attributes. Access to this is provided so that attribute authoring
    /// happening inside non-member functions can make use of it.
    MAYAUSD_CORE_PUBLIC
    UsdUtilsSparseValueWriter* _GetSparseValueWriter();

    UsdPrim                 _usdPrim;
    UsdMayaWriteJobContext& _writeJobCtx;

private:
    /// Whether this prim writer represents the transform portion of a merged
    /// shape and transform.
    bool _IsMergedTransform() const;

    /// Whether this prim writer represents the shape portion of a merged shape
    /// and transform.
    bool _IsMergedShape() const;

    /// The MDagPath for the Maya node being written, valid only for DAG node
    /// prim writers.
    const MDagPath _dagPath;

    /// The MObject for the Maya node being written, valid for both DAG and DG
    /// node prim writers.
    const MObject _mayaObject;

    const SdfPath                           _usdPath;
    const UsdMayaUtil::MDagPathMap<SdfPath> _baseDagToUsdPaths;

    UsdUtilsSparseValueWriter _valueWriter;

    bool _exportVisibility;
    bool _hasAnimCurves;
};

typedef std::shared_ptr<UsdMayaPrimWriter> UsdMayaPrimWriterSharedPtr;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
