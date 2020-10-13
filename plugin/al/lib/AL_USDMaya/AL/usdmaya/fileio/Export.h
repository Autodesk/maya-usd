//
// Copyright 2017 Animal Logic
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
#pragma once
#include "AL/maya/utils/Api.h"
#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/usdmaya/fileio/ExportParams.h"
#include "AL/usdmaya/fileio/translators/TranslatorBase.h"
#include "AL/usdmaya/utils/ForwardDeclares.h"

#include <pxr/pxr.h>

#include <maya/MPxCommand.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace fileio {

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A class that wraps up the entire export process
///
/// This command will export the maya hierarchy at the specified prim path or selection, into the
/// USD format. During traversal of the maya DAG hierarchy, the USD schema type is exported on the
/// prim if it can be determined from the maya node type, if the schema type couldn't be determined
/// a Xform type prim is exported to represent it. If a known type is found, then it is exported via
/// the the corresponding export translator which is configured based on the input parameters of the
/// command.
///
/// Each exporter has an explicit list of plugs that will get exported if certain input parameters
/// are passed in then they can be exported over a frame range
///
/// Instances are currently not fully supported. If instances are encountered and the
/// duplicateInstances flag is ON, then the hierarchy is deduplicated. If duplicateInstances is off
/// then only the first instance found is exported. \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class Export
{
public:
    /// \brief  The constructor performs the export from Maya to USD based on the input parameters.
    /// \param  params the export parameters that describe what will be exported from Maya
    Export(const ExporterParams& params);

    /// \brief  dtor
    ~Export();

private:
    enum ReferenceType
    {
        kNoReference,
        kShapeReference,
        kTransformReference,
    };

    void exportSceneHierarchy(MDagPath path, SdfPath& defaultPrim);
    void exportShapesCommonProc(
        MDagPath      shapePath,
        MFnTransform& fnTransform,
        SdfPath&      usdPath,
        ReferenceType refType,
        bool          exportInWorldSpace);
    void    exportShapesOnlyUVProc(MDagPath shapePath, MFnTransform& fnTransform, SdfPath& usdPath);
    UsdPrim exportMeshUV(MDagPath path, const SdfPath& usdPath);
    UsdPrim exportAssembly(MDagPath path, const SdfPath& usdPath);
    UsdPrim exportPluginLocatorNode(MDagPath path, const SdfPath& usdPath);
    UsdPrim exportPluginShape(MDagPath path, const SdfPath& usdPath);
    void    exportIkChain(MDagPath effectorPath, const SdfPath& usdPath);
    void    exportGeometryConstraint(MDagPath effectorPath, const SdfPath& usdPath);
    void    copyTransformParams(UsdPrim prim, MFnTransform& fnTransform, bool exportInWorldSpace);
    SdfPath determineUsdPath(MDagPath path, const SdfPath& usdPath, ReferenceType refType);
    void    addReferences(
           MDagPath       shapePath,
           MFnTransform&  fnTransform,
           SdfPath&       usdPath,
           const SdfPath& instancePath,
           ReferenceType  refType,
           bool           exportInWorldSpace);
    inline bool isPrimDefined(SdfPath& usdPath);
    struct Impl;
    void                               doExport();
    const ExporterParams&              m_params;
    Impl*                              m_impl;
    translators::TranslatorManufacture m_translatorManufacture;
};

//----------------------------------------------------------------------------------------------------------------------
/// \brief  A thin MEL command layer that just wraps the AL::usdmaya::fileio::Export process.
/// \ingroup   fileio
//----------------------------------------------------------------------------------------------------------------------
class ExportCommand : public MPxCommand
{
public:
    AL_MAYA_DECLARE_COMMAND();

    /// \brief  ctor
    ExportCommand();

    /// \brief  dtor
    ~ExportCommand();

private:
    MStatus        doIt(const MArgList& args);
    MStatus        redoIt();
    MStatus        undoIt();
    bool           isUndoable() const { return true; };
    ExporterParams m_params;
};

//----------------------------------------------------------------------------------------------------------------------
} // namespace fileio
} // namespace usdmaya
} // namespace AL
//----------------------------------------------------------------------------------------------------------------------
