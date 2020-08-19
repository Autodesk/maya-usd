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
#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/shaderReader.h>
#include <mayaUsd/fileio/shaderReaderRegistry.h>
#include <mayaUsd/fileio/shaderWriter.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/input.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/output.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdShade/tokens.h>

#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnSet.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

using _NodeHandleToShaderWriterMap =
    UsdMayaUtil::MObjectHandleUnorderedMap<UsdMayaShaderWriterSharedPtr>;

class UseRegistryShadingModeExporter : public UsdMayaShadingModeExporter
{
    public:

        UseRegistryShadingModeExporter() {}

    private:

        /// Gets a shader writer for \p depNode that authors its prim(s) under
        /// the path \p parentPath.
        ///
        /// If no shader writer can be found for the Maya node or if the node
        /// otherwise should not be authored, an empty pointer is returned.
        ///
        /// A cached mapping of node handles to shader writer pointers is
        /// maintained in the provided \p shaderWriterMap.
        UsdMayaShaderWriterSharedPtr
        _GetShaderWriterForNode(
            const MObject& depNode,
            const SdfPath& parentPath,
            const UsdMayaShadingModeExportContext& context,
            _NodeHandleToShaderWriterMap& shaderWriterMap)
        {
            if (depNode.hasFn(MFn::kShadingEngine)) {
                // depNode is the material itself, so we don't need to create a
                // new shader. Connections between it and the top-level shader
                // will be handled by the main Export() method.
                return nullptr;
            }

            if (depNode.hasFn(MFn::kDagNode)) {
                // XXX: Skip DAG nodes for now, but we may eventually want/need
                // to consider them.
                return nullptr;
            }

            if (!UsdMayaUtil::isWritable(depNode)) {
                return nullptr;
            }

            const MObjectHandle nodeHandle(depNode);
            const auto iter = shaderWriterMap.find(nodeHandle);
            if (iter != shaderWriterMap.end()) {
                // We've already created a shader writer for this node, so just
                // return it.
                return iter->second;
            }

            // No shader writer exists for this node yet, so create one.
            MStatus status;
            const MFnDependencyNode depNodeFn(depNode, &status);
            if (status != MS::kSuccess) {
                return nullptr;
            }

            const TfToken shaderUsdPrimName(
                UsdMayaUtil::SanitizeName(depNodeFn.name().asChar()));

            const SdfPath shaderUsdPath =
                parentPath.AppendChild(shaderUsdPrimName);

            UsdMayaPrimWriterSharedPtr primWriter =
                context.GetWriteJobContext().CreatePrimWriter(
                    depNodeFn,
                    shaderUsdPath);

            UsdMayaShaderWriterSharedPtr shaderWriter =
                std::dynamic_pointer_cast<UsdMayaShaderWriter>(primWriter);

            // Store the shader writer pointer whether we succeeded or not so
            // that we don't repeatedly attempt and fail to create it for the
            // same node.
            shaderWriterMap[nodeHandle] = shaderWriter;

            return shaderWriter;
        }

        /// Export nodes in the Maya dependency graph rooted at \p rootPlug
        /// for \p material.
        ///
        /// The root plug should be from an attribute on the Maya shadingEngine
        /// node that \p material represents.
        ///
        /// The first shader prim authored during the traversal will be assumed
        /// to be the primary shader for the connection represented by
        /// \p rootPlug. That shader prim will be returned so that it can be
        /// connected to the Material prim.
        UsdShadeShader
        _ExportShadingDepGraph(
                UsdShadeMaterial& material,
                const MPlug& rootPlug,
                const UsdMayaShadingModeExportContext& context)
        {
            // Maintain a mapping of Maya shading node handles to shader
            // writers so that we only author each shader once, but can still
            // look them up again to create connections.
            _NodeHandleToShaderWriterMap shaderWriterMap;

            // MItDependencyGraph takes a non-const MPlug as a constructor
            // parameter, so we have to make a copy of rootPlug here.
            MPlug rootPlugCopy(rootPlug);

            MStatus status;
            MItDependencyGraph iterDepGraph(
                rootPlugCopy,
                MFn::kInvalid,
                MItDependencyGraph::Direction::kUpstream,
                MItDependencyGraph::Traversal::kDepthFirst,
                MItDependencyGraph::Level::kPlugLevel,
                &status);
            if (status != MS::kSuccess) {
                return UsdShadeShader();
            }

            // We'll consider the first shader we create to be the "top-level"
            // shader, which will be the one we return so that it can be
            // connected to the Material prim.
            UsdShadeShader topLevelShader;

            for (; !iterDepGraph.isDone(); iterDepGraph.next()) {
                const MPlug iterPlug = iterDepGraph.thisPlug(&status);
                if (status != MS::kSuccess) {
                    continue;
                }

                // We'll check the source and the destination(s) of the
                // connection to see if we encounter new shading nodes that
                // need to be exported.
                MPlug srcPlug;
                MPlugArray dstPlugs;

                const bool isDestination = iterPlug.isDestination(&status);
                if (status != MS::kSuccess) {
                    continue;
                }
                const bool isSource = iterPlug.isSource(&status);
                if (status != MS::kSuccess) {
                    continue;
                }

                if (isDestination) {
                    srcPlug = iterPlug.source(&status);
                    if (status != MS::kSuccess) {
                        continue;
                    }

                    dstPlugs.append(iterPlug);
                } else if (isSource) {
                    srcPlug = iterPlug;

                    if (!iterPlug.destinations(dstPlugs, &status) ||
                            status != MS::kSuccess) {
                        continue;
                    }
                }

                // Since we are traversing the shading graph in the upstream
                // direction, we'll be visiting shading nodes from destinations
                // to sources, beginning with the shadingEngine node. This
                // means that if we don't have a source shader to work with,
                // there's no need to consider any of the plug's destinations.
                if (srcPlug.isNull()) {
                    continue;
                }

                UsdMayaShaderWriterSharedPtr srcShaderWriter =
                    _GetShaderWriterForNode(
                        srcPlug.node(),
                        material.GetPath(),
                        context,
                        shaderWriterMap);
                if (!srcShaderWriter) {
                    continue;
                }

                srcShaderWriter->Write(UsdTimeCode::Default());

                UsdPrim shaderPrim = srcShaderWriter->GetUsdPrim();
                if (shaderPrim && !topLevelShader) {
                    topLevelShader = UsdShadeShader(shaderPrim);
                }

                for (unsigned int i = 0u; i < dstPlugs.length(); ++i) {
                    const MPlug dstPlug = dstPlugs[i];
                    if (dstPlug.isNull()) {
                        continue;
                    }

                    UsdMayaShaderWriterSharedPtr dstShaderWriter =
                        _GetShaderWriterForNode(
                            dstPlug.node(),
                            material.GetPath(),
                            context,
                            shaderWriterMap);
                    if (!dstShaderWriter) {
                        continue;
                    }

                    dstShaderWriter->Write(UsdTimeCode::Default());

                    UsdPrim shaderPrim = dstShaderWriter->GetUsdPrim();
                    if (shaderPrim && !topLevelShader) {
                        topLevelShader = UsdShadeShader(shaderPrim);
                    }

                    // See if we can get the USD shading attributes that the
                    // Maya plugs represent so that we can author the
                    // connection in USD.

                    const TfToken srcPlugName =
                        TfToken(context.GetStandardAttrName(srcPlug, false));
                    UsdAttribute srcAttribute =
                        srcShaderWriter->GetShadingAttributeForMayaAttrName(
                            srcPlugName);

                    const TfToken dstPlugName =
                        TfToken(context.GetStandardAttrName(dstPlug, false));
                    UsdAttribute dstAttribute =
                        dstShaderWriter->GetShadingAttributeForMayaAttrName(
                            dstPlugName);

                    if (srcAttribute && dstAttribute) {
                        if (UsdShadeInput::IsInput(srcAttribute)) {
                            UsdShadeInput srcInput(srcAttribute);

                            UsdShadeConnectableAPI::ConnectToSource(
                                dstAttribute,
                                srcInput);
                        }
                        else if (UsdShadeOutput::IsOutput(srcAttribute)) {
                            UsdShadeOutput srcOutput(srcAttribute);

                            UsdShadeConnectableAPI::ConnectToSource(
                                dstAttribute,
                                srcOutput);
                        }
                    }
                }
            }

            return topLevelShader;
        }

        void
        Export(
                const UsdMayaShadingModeExportContext& context,
                UsdShadeMaterial* const mat,
                SdfPathSet* const boundPrimPaths) override
        {
            MStatus status;

            MObject shadingEngine = context.GetShadingEngine();
            const MFnDependencyNode shadingEngineDepNodeFn(
                shadingEngine,
                &status);
            if (status != MS::kSuccess) {
                TF_RUNTIME_ERROR(
                    "Cannot export invalid shading engine node '%s'\n",
                    UsdMayaUtil::GetMayaNodeName(shadingEngine).c_str());
                return;
            }

            const UsdMayaShadingModeExportContext::AssignmentVector& assignments =
                context.GetAssignments();
            if (assignments.empty()) {
                return;
            }

            UsdPrim materialPrim =
                context.MakeStandardMaterialPrim(
                    assignments,
                    std::string(),
                    boundPrimPaths);
            UsdShadeMaterial material(materialPrim);
            if (!material) {
                return;
            }

            if (mat != nullptr) {
                *mat = material;
            }

            UsdShadeShader surfaceShaderSchema =
                _ExportShadingDepGraph(
                    material,
                    context.GetSurfaceShaderPlug(),
                    context);
            UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
                surfaceShaderSchema,
                UsdShadeTokens->surface,
                SdfValueTypeNames->Token,
                material,
                UsdShadeTokens->surface);

            UsdShadeShader volumeShaderSchema =
                _ExportShadingDepGraph(
                    material,
                    context.GetVolumeShaderPlug(),
                    context);
            UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
                volumeShaderSchema,
                UsdShadeTokens->volume,
                SdfValueTypeNames->Token,
                material,
                UsdShadeTokens->volume);

            UsdShadeShader displacementShaderSchema =
                _ExportShadingDepGraph(
                    material,
                    context.GetDisplacementShaderPlug(),
                    context);
            UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
                displacementShaderSchema,
                UsdShadeTokens->displacement,
                SdfValueTypeNames->Token,
                material,
                UsdShadeTokens->displacement);
        }
};

} // anonymous namespace

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeExportContext, useRegistry)
{
    UsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "useRegistry",
        []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(
                static_cast<UsdMayaShadingModeExporter*>(
                    new UseRegistryShadingModeExporter()));
        }
    );
}

namespace {

/// This class implements a shading mode importer which uses a registry keyed by the info:id USD
/// attribute to provide an importer class for each UsdShade node processed while traversing the
/// main connections of a UsdMaterial node.
class UseRegistryShadingModeImporter {
public:
    UseRegistryShadingModeImporter(UsdMayaShadingModeImportContext* context,
                                   const UsdMayaJobImportArgs& jobArguments)
        : _context(context)
        , _jobArguments(jobArguments)
    {
    }

    /// Main entry point of the import process. On input we get a UsdMaterial which gets traversed
    /// in order to build a Maya shading network that reproduces the information found in the USD
    /// shading network.
    MObject Read()
    {
        const UsdShadeMaterial& shadeMaterial = _context->GetShadeMaterial();
        if (!shadeMaterial) {
            return MObject();
        }

        UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource();
        if (!surfaceShader) {
            return MObject();
        }

        const TfToken surfaceShaderPlugName = _context->GetSurfaceShaderPlugName();
        if (surfaceShaderPlugName.IsEmpty()) {
            return MObject();
        }

        MPlug shaderOutputPlug = _GetSourcePlug(surfaceShader, UsdShadeTokens->surface);
        if (shaderOutputPlug.isNull()) {
            return MObject();
        }

        // Create the shading engine.
        MObject shadingEngine = _context->CreateShadingEngine();
        if (shadingEngine.isNull()) {
            return MObject();
        }
        MStatus status;
        MFnSet  fnSet(shadingEngine, &status);
        if (status != MS::kSuccess) {
            return MObject();
        }

        MPlug seInputPlug = fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());

        UsdMayaUtil::Connect(
            shaderOutputPlug,
            seInputPlug,
            /* clearDstPlug = */ true);

        return shadingEngine;
    }

private:
    /// Gets the Maya-side source plug that corresponds to the \p outputName attribute of
    /// \p shaderSchema.
    ///
    /// This will create the Maya dependency nodes as necessary and return an empty plug in case of
    /// import failure or if \p outputName could not map to a Maya plug.
    ///
    MPlug _GetSourcePlug(const UsdShadeShader& shaderSchema, const TfToken& outputName) {
        const SdfPath& shaderPath(shaderSchema.GetPath());
        const auto     iter = _shaderReaderMap.find(shaderPath);
        UsdMayaShaderReaderSharedPtr shaderReader;
        MObject sourceObj;
        if (iter != _shaderReaderMap.end()) {
            shaderReader = iter->second;
            if (!_context->GetCreatedObject(shaderSchema.GetPrim(), &sourceObj)) {
                return MPlug();
            }
        } else {
            // No shader reader exists for this node yet, so create one.
            TfToken shaderId;
            shaderSchema.GetIdAttr().Get(&shaderId);

            if (UsdMayaShaderReaderRegistry::ReaderFactoryFn factoryFn
                = UsdMayaShaderReaderRegistry::Find(shaderId, _jobArguments)) {

                UsdPrim shaderPrim = shaderSchema.GetPrim();
                UsdMayaPrimReaderArgs args(shaderPrim, _jobArguments);

                shaderReader = std::dynamic_pointer_cast<UsdMayaShaderReader>(factoryFn(args));

                sourceObj = _ReadSchema(shaderSchema, *shaderReader);
                if (sourceObj.isNull()) {
                    // Read failed. Invalidate the reader.
                    shaderReader = nullptr;
                }
            }

            // Store the shader reader pointer whether we succeeded or not so
            // that we don't repeatedly attempt and fail to create it for the
            // same node.
            _shaderReaderMap[shaderPath] = shaderReader;

            if (!shaderReader) {
                return MPlug();
            }
        }

        MStatus status;
        MFnDependencyNode sourceDepFn(sourceObj, &status);
        if (status != MS::kSuccess) {
            return MPlug();
        }

        TfToken sourceOutputName
            = TfToken(TfStringPrintf(
                            "%s%s", UsdShadeTokens->outputs.GetText(), outputName.GetText())
                            .c_str());
        auto  srcAttrName = shaderReader->GetMayaNameForUsdAttrName(sourceOutputName);
        MPlug sourcePlug = sourceDepFn.findPlug(srcAttrName.GetText());
        if (sourcePlug.isArray()) {
            const unsigned int numElements = sourcePlug.evaluateNumElements();
            if (numElements > 0u) {
                if (numElements > 1u) {
                    TF_WARN(
                        "Array with multiple elements encountered at '%s'. "
                        "Currently, only arrays with a single element are "
                        "supported. Not connecting attribute.",
                        sourcePlug.name().asChar());
                    return MPlug();
                }

                sourcePlug = sourcePlug[0];
            }
        }

        return sourcePlug;
    }

    /// Reads \p shaderSchema using \p shaderReader.
    ///
    /// This will create the Maya dependency nodes for the \p shaderSchema UsdShade node. The
    /// connections will be recursively traversed to complete the network.
    ///
    MObject
    _ReadSchema(const UsdShadeShader& shaderSchema, UsdMayaShaderReader& shaderReader)
    {
        // UsdMayaPrimReader::Read is a function that works by indirect effect. It will return
        // "true" on success, and the resulting changes will be found in the _context object.
        if (!shaderReader.Read(_context->GetPrimReaderContext())) {
            return MObject();
        }

        MObject shaderObj;
        if (!_context->GetCreatedObject(shaderSchema.GetPrim(), &shaderObj)) {
            return MObject();
        }

        MStatus           status;
        MFnDependencyNode depFn(shaderObj, &status);
        if (status != MS::kSuccess) {
            return MObject();
        }

        for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
            auto mayaAttrName = shaderReader.GetMayaNameForUsdAttrName(input.GetFullName());
            if (mayaAttrName.IsEmpty()) {
                continue;
            }
            MPlug                  mayaAttr = depFn.findPlug(mayaAttrName.GetText());
            UsdShadeConnectableAPI source;
            TfToken                sourceOutputName;
            UsdShadeAttributeType  sourceType;

            // Follow shader connections and recurse.
            if (!UsdShadeConnectableAPI::GetConnectedSource(
                    input, &source, &sourceOutputName, &sourceType)) {
                continue;
            }

            UsdShadeShader sourceShaderSchema = UsdShadeShader(source.GetPrim());
            if (!sourceShaderSchema) {
                continue;
            }

            MPlug srcAttr = _GetSourcePlug(sourceShaderSchema, sourceOutputName);
            if (srcAttr.isNull()) {
                continue;
            }

            UsdMayaUtil::Connect(srcAttr, mayaAttr, false);
        }

        return shaderObj;
    }

    using _SdfPathToShaderReaderMap
        = std::unordered_map<SdfPath, UsdMayaShaderReaderSharedPtr, SdfPath::Hash>;

    UsdMayaShadingModeImportContext* _context = nullptr;
    const UsdMayaJobImportArgs&      _jobArguments;
    _SdfPathToShaderReaderMap        _shaderReaderMap;
};

}; // anonymous namespace

DEFINE_SHADING_MODE_IMPORTER_WITH_JOB_ARGUMENTS(useRegistry, context, jobArguments)
{
    UseRegistryShadingModeImporter importer(context, jobArguments);
    return importer.Read();
}

PXR_NAMESPACE_CLOSE_SCOPE
