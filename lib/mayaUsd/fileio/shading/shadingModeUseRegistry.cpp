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
#include <mayaUsd/fileio/shaderWriterRegistry.h>
#include <mayaUsd/fileio/shading/shadingModeExporter.h>
#include <mayaUsd/fileio/shading/shadingModeExporterContext.h>
#include <mayaUsd/fileio/shading/shadingModeRegistry.h>
#include <mayaUsd/fileio/utils/shadingUtil.h>
#include <mayaUsd/utils/converter.h>
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

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((ArgName, "useRegistry"))
    ((NiceName, "Use Registry"))
    ((ExportDescription,
        "Use a registry based mechanism, complemented with material conversions,"
         " to export to a UsdShade network"))
    ((ImportDescription,
         "Use a registry based mechanism, complemented with material conversions,"
         " to import from a UsdShade network"))
);
// clang-format on

using _NodeHandleToShaderWriterMap
    = UsdMayaUtil::MObjectHandleUnorderedMap<UsdMayaShaderWriterSharedPtr>;

class UseRegistryShadingModeExporter : public UsdMayaShadingModeExporter
{
public:
    UseRegistryShadingModeExporter() { }

private:
    /// Gets the exported ShadeNode associated with the \p depNode that was written under
    /// the path \p parentPath. If no such node exists, then one is created and written.
    ///
    /// If no shader writer can be found for the Maya node or if the node
    /// otherwise should not be authored, an empty pointer is returned.
    ///
    /// A cached mapping of node handles to shader writer pointers is
    /// maintained in the provided \p shaderWriterMap.
    UsdMayaShaderWriterSharedPtr _GetExportedShaderForNode(
        const MObject&                         depNode,
        const SdfPath&                         parentPath,
        const UsdMayaShadingModeExportContext& context,
        _NodeHandleToShaderWriterMap&          shaderWriterMap)
    {
        if (depNode.hasFn(MFn::kShadingEngine)) {
            // depNode is the material itself, so we don't need to create a
            // new shader. Connections between it and the top-level shader
            // will be handled by the main Export() method.
            return nullptr;
        }

        if (!UsdMayaUtil::isWritable(depNode)) {
            return nullptr;
        }

        const MObjectHandle nodeHandle(depNode);
        const auto          iter = shaderWriterMap.find(nodeHandle);
        if (iter != shaderWriterMap.end()) {
            // We've already created a shader writer for this node, so just
            // return it.
            return iter->second;
        }

        // No shader writer exists for this node yet, so create one.
        MStatus                 status;
        const MFnDependencyNode depNodeFn(depNode, &status);
        if (status != MS::kSuccess) {
            return nullptr;
        }

        const TfToken shaderUsdPrimName(UsdMayaUtil::SanitizeName(depNodeFn.name().asChar()));

        const SdfPath shaderUsdPath = parentPath.AppendChild(shaderUsdPrimName);

        UsdMayaShaderWriterRegistry::WriterFactoryFn primWriterFactory
            = UsdMayaShaderWriterRegistry::Find(
                TfToken(depNodeFn.typeName().asChar()), context.GetExportArgs());
        if (!primWriterFactory) {
            return nullptr;
        }

        UsdMayaPrimWriterSharedPtr primWriter
            = primWriterFactory(depNodeFn, shaderUsdPath, context.GetWriteJobContext());
        if (!primWriter) {
            return nullptr;
        }

        UsdMayaShaderWriterSharedPtr shaderWriter
            = std::dynamic_pointer_cast<UsdMayaShaderWriter>(primWriter);

        // Store the shader writer pointer whether we succeeded or not so
        // that we don't repeatedly attempt and fail to create it for the
        // same node.
        shaderWriterMap[nodeHandle] = shaderWriter;

        shaderWriter->Write(UsdTimeCode::Default());

        return shaderWriter;
    }

    /// Export nodes in the Maya dependency graph rooted at \p rootPlug
    /// under \p materialExportPath.
    ///
    /// The root plug should be from an attribute on the Maya shadingEngine
    /// node that the material represents.
    ///
    /// The first shader prim authored during the traversal will be assumed
    /// to be the primary shader for the connection represented by
    /// \p rootPlug. That shader prim will be returned so that it can be
    /// connected to the Material prim.
    UsdShadeShader _ExportShadingDepGraph(
        const SdfPath&                         materialExportPath,
        const MPlug&                           rootPlug,
        const UsdMayaShadingModeExportContext& context)
    {
        // Maintain a mapping of Maya shading node handles to shader
        // writers so that we only author each shader once, but can still
        // look them up again to create connections.
        _NodeHandleToShaderWriterMap shaderWriterMap;

        // MItDependencyGraph takes a non-const MPlug as a constructor
        // parameter, so we have to make a copy of rootPlug here.
        MPlug rootPlugCopy(rootPlug);

        MStatus            status;
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
            MPlug      srcPlug;
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

                if (!iterPlug.destinations(dstPlugs, &status) || status != MS::kSuccess) {
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

            auto srcShaderInfo = _GetExportedShaderForNode(
                srcPlug.node(), materialExportPath, context, shaderWriterMap);
            if (!srcShaderInfo) {
                continue;
            }

            UsdPrim shaderPrim = srcShaderInfo->GetUsdPrim();
            if (shaderPrim && !topLevelShader) {
                topLevelShader = UsdShadeShader(shaderPrim);
            }

            for (unsigned int i = 0u; i < dstPlugs.length(); ++i) {
                const MPlug dstPlug = dstPlugs[i];
                if (dstPlug.isNull()) {
                    continue;
                }

                auto dstShaderInfo = _GetExportedShaderForNode(
                    dstPlug.node(), materialExportPath, context, shaderWriterMap);
                if (!dstShaderInfo) {
                    continue;
                }

                UsdPrim shaderPrim = dstShaderInfo->GetUsdPrim();
                if (shaderPrim && !topLevelShader) {
                    topLevelShader = UsdShadeShader(shaderPrim);
                }

                // See if we can get the USD shading attributes that the
                // Maya plugs represent so that we can author the
                // connection in USD.

                // We pass in the type of the plug on the other side to allow the export code to
                // add conversion nodes as required.
                const TfToken dstPlugName
                    = TfToken(UsdMayaShadingUtil::GetStandardAttrName(dstPlug, false));
                UsdAttribute dstAttribute = dstShaderInfo->GetShadingAttributeForMayaAttrName(
                    dstPlugName, MayaUsd::Converter::getUsdTypeName(srcPlug));

                UsdAttribute srcAttribute;
                if (dstAttribute) {
                    const TfToken srcPlugName
                        = TfToken(UsdMayaShadingUtil::GetStandardAttrName(srcPlug, false));
                    srcAttribute = srcShaderInfo->GetShadingAttributeForMayaAttrName(
                        srcPlugName, dstAttribute.GetTypeName());
                }

                if (srcAttribute && dstAttribute) {
                    if (UsdShadeInput::IsInput(srcAttribute)) {
                        UsdShadeInput srcInput(srcAttribute);

                        UsdShadeConnectableAPI::ConnectToSource(dstAttribute, srcInput);
                    } else if (UsdShadeOutput::IsOutput(srcAttribute)) {
                        UsdShadeOutput srcOutput(srcAttribute);

                        UsdShadeConnectableAPI::ConnectToSource(dstAttribute, srcOutput);
                    }
                }
            }
        }

        return topLevelShader;
    }

    void Export(
        const UsdMayaShadingModeExportContext& context,
        UsdShadeMaterial* const                mat,
        SdfPathSet* const                      boundPrimPaths) override
    {
        MStatus status;

        MObject                 shadingEngine = context.GetShadingEngine();
        const MFnDependencyNode shadingEngineDepNodeFn(shadingEngine, &status);
        if (status != MS::kSuccess) {
            TF_RUNTIME_ERROR(
                "Cannot export invalid shading engine node '%s'\n",
                UsdMayaUtil::GetMayaNodeName(shadingEngine).c_str());
            return;
        }

        const UsdMayaShadingModeExportContext::AssignmentVector& assignments
            = context.GetAssignments();
        if (assignments.empty()) {
            return;
        }

        UsdPrim materialPrim = context.MakeStandardMaterialPrim(assignments, std::string());
        UsdShadeMaterial material(materialPrim);
        if (!material) {
            return;
        }

        if (mat != nullptr) {
            *mat = material;
        }

        const TfToken& convertMaterialsTo = context.GetExportArgs().convertMaterialsTo;
        const TfToken& renderContext
            = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(convertMaterialsTo)
                  .renderContext;
        SdfPath materialExportPath = materialPrim.GetPath();

        UsdShadeShader surfaceShaderSchema
            = _ExportShadingDepGraph(materialExportPath, context.GetSurfaceShaderPlug(), context);
        UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
            surfaceShaderSchema, material, UsdShadeTokens->surface, renderContext);

        UsdShadeShader volumeShaderSchema
            = _ExportShadingDepGraph(materialExportPath, context.GetVolumeShaderPlug(), context);
        UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
            volumeShaderSchema, material, UsdShadeTokens->volume, renderContext);

        UsdShadeShader displacementShaderSchema = _ExportShadingDepGraph(
            materialExportPath, context.GetDisplacementShaderPlug(), context);
        UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
            displacementShaderSchema, material, UsdShadeTokens->displacement, renderContext);

        context.BindStandardMaterialPrim(materialPrim, assignments, boundPrimPaths);
    }
};

} // anonymous namespace

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeExportContext, useRegistry)
{
    UsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        _tokens->ArgName.GetString(),
        _tokens->NiceName.GetString(),
        _tokens->ExportDescription.GetString(),
        []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(
                static_cast<UsdMayaShadingModeExporter*>(new UseRegistryShadingModeExporter()));
        });
}

namespace {

/// This class implements a shading mode importer which uses a registry keyed by the info:id USD
/// attribute to provide an importer class for each UsdShade node processed while traversing the
/// main connections of a UsdMaterial node.
class UseRegistryShadingModeImporter
{
public:
    UseRegistryShadingModeImporter(
        UsdMayaShadingModeImportContext* context,
        const UsdMayaJobImportArgs&      jobArguments)
        : _context(context)
        , _jobArguments(jobArguments)
    {
    }

    /// Main entry point of the import process. On input we get a UsdMaterial which gets traversed
    /// in order to build a Maya shading network that reproduces the information found in the USD
    /// shading network.
    MObject Read()
    {
        if (_jobArguments.shadingModes.size() != 1) {
            // The material translator will make sure we only get a single shading mode
            // at a time.
            TF_CODING_ERROR("useRegistry importer can only handle a single shadingMode");
            return MObject();
        }
        const TfToken& materialConversion = _jobArguments.GetMaterialConversion();
        TfToken        renderContext
            = UsdMayaShadingModeRegistry::GetMaterialConversionInfo(materialConversion)
                  .renderContext;

        const UsdShadeMaterial& shadeMaterial = _context->GetShadeMaterial();
        if (!shadeMaterial) {
            return MObject();
        }

        MPlug surfaceOutputPlug, volumeOutputPlug, displacementOutputPlug;

        // ComputeSurfaceSource will default to the universal render context if
        // renderContext is not found. Therefore we need to test first that the
        // render context output we are looking for really exists:
        if (shadeMaterial.GetSurfaceOutput(renderContext)) {
#if PXR_VERSION > 2105
            UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource({ renderContext });
#else
            UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource(renderContext);
#endif
            if (surfaceShader) {
                const TfToken surfaceShaderPlugName = _context->GetSurfaceShaderPlugName();
                if (!surfaceShaderPlugName.IsEmpty()) {
                    surfaceOutputPlug = _GetSourcePlug(surfaceShader, UsdShadeTokens->surface);
                }
            }
        }

        if (shadeMaterial.GetVolumeOutput(renderContext)) {
#if PXR_VERSION > 2105
            UsdShadeShader volumeShader = shadeMaterial.ComputeVolumeSource({ renderContext });
#else
            UsdShadeShader volumeShader = shadeMaterial.ComputeVolumeSource(renderContext);
#endif
            if (volumeShader) {
                const TfToken volumeShaderPlugName = _context->GetVolumeShaderPlugName();
                if (!volumeShaderPlugName.IsEmpty()) {
                    volumeOutputPlug = _GetSourcePlug(volumeShader, UsdShadeTokens->volume);
                }
            }
        }

        if (shadeMaterial.GetDisplacementOutput(renderContext)) {
#if PXR_VERSION > 2105
            UsdShadeShader displacementShader
                = shadeMaterial.ComputeDisplacementSource({ renderContext });
#else
            UsdShadeShader displacementShader
                = shadeMaterial.ComputeDisplacementSource(renderContext);
#endif
            if (displacementShader) {
                const TfToken displacementShaderPlugName
                    = _context->GetDisplacementShaderPlugName();
                if (!displacementShaderPlugName.IsEmpty()) {
                    displacementOutputPlug
                        = _GetSourcePlug(displacementShader, UsdShadeTokens->displacement);
                }
            }
        }

        if (surfaceOutputPlug.isNull() && volumeOutputPlug.isNull()
            && displacementOutputPlug.isNull()) {
            return MObject();
        }

        // Create the shading engine.
        std::string       surfaceNodeName;
        MFnDependencyNode surfaceNodeFn;
        if (surfaceNodeFn.setObject(surfaceOutputPlug.node()) == MS::kSuccess) {
            surfaceNodeName = surfaceNodeFn.name().asChar();
        }
        MObject shadingEngine = _context->CreateShadingEngine(surfaceNodeName);
        if (shadingEngine.isNull()) {
            return MObject();
        }
        MStatus status;
        MFnSet  fnSet(shadingEngine, &status);
        if (status != MS::kSuccess) {
            return MObject();
        }

        if (!surfaceOutputPlug.isNull()) {
            MPlug seInputPlug
                = fnSet.findPlug(_context->GetSurfaceShaderPlugName().GetText(), &status);
            CHECK_MSTATUS_AND_RETURN(status, MObject());
            UsdMayaUtil::Connect(surfaceOutputPlug, seInputPlug, true);
        }

        if (!volumeOutputPlug.isNull()) {
            MPlug veInputPlug
                = fnSet.findPlug(_context->GetVolumeShaderPlugName().GetText(), &status);
            CHECK_MSTATUS_AND_RETURN(status, MObject());
            UsdMayaUtil::Connect(volumeOutputPlug, veInputPlug, true);
        }

        if (!displacementOutputPlug.isNull()) {
            MPlug deInputPlug
                = fnSet.findPlug(_context->GetDisplacementShaderPlugName().GetText(), &status);
            CHECK_MSTATUS_AND_RETURN(status, MObject());
            UsdMayaUtil::Connect(displacementOutputPlug, deInputPlug, true);
        }

        return shadingEngine;
    }

private:
    /// Gets the Maya-side source plug that corresponds to the \p outputName attribute of
    /// \p shaderSchema.
    ///
    /// This will create the Maya dependency nodes as necessary and return an empty plug in case of
    /// import failure or if \p outputName could not map to a Maya plug.
    ///
    MPlug _GetSourcePlug(const UsdShadeShader& shaderSchema, const TfToken& outputName)
    {
        const SdfPath&               shaderPath(shaderSchema.GetPath());
        const auto                   iter = _shaderReaderMap.find(shaderPath);
        UsdMayaShaderReaderSharedPtr shaderReader;
        MObject                      sourceObj;
        MPlug                        sourcePlug;
        if (iter != _shaderReaderMap.end()) {
            shaderReader = iter->second;
            sourceObj = shaderReader->GetCreatedObject(*_context, shaderSchema.GetPrim());
            if (sourceObj.isNull()) {
                return MPlug();
            }
        } else {
            // No shader reader exists for this node yet, so create one.
            TfToken shaderId;
            shaderSchema.GetIdAttr().Get(&shaderId);

            if (UsdMayaShaderReaderRegistry::ReaderFactoryFn factoryFn
                = UsdMayaShaderReaderRegistry::Find(shaderId, _jobArguments)) {

                UsdPrim               shaderPrim = shaderSchema.GetPrim();
                UsdMayaPrimReaderArgs args(shaderPrim, _jobArguments);

                shaderReader = std::dynamic_pointer_cast<UsdMayaShaderReader>(factoryFn(args));

                UsdShadeShader downstreamSchema;
                TfToken        downstreamName;
                if (shaderReader->IsConverter(downstreamSchema, downstreamName)) {
                    // Recurse downstream:
                    sourcePlug = _GetSourcePlug(downstreamSchema, downstreamName);
                    if (!sourcePlug.isNull()) {
                        shaderReader->SetDownstreamReader(
                            _shaderReaderMap[downstreamSchema.GetPath()]);
                        sourceObj = sourcePlug.node();
                    } else {
                        // Read failed. Invalidate the reader.
                        shaderReader = nullptr;
                    }
                } else {
                    sourceObj = _ReadSchema(shaderSchema, *shaderReader);
                    if (sourceObj.isNull()) {
                        // Read failed. Invalidate the reader.
                        shaderReader = nullptr;
                    }
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

        TfToken sourceOutputName = TfToken(
            TfStringPrintf("%s%s", UsdShadeTokens->outputs.GetText(), outputName.GetText())
                .c_str());
        sourcePlug = shaderReader->GetMayaPlugForUsdAttrName(sourceOutputName, sourceObj);
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
    MObject _ReadSchema(const UsdShadeShader& shaderSchema, UsdMayaShaderReader& shaderReader)
    {
        // UsdMayaPrimReader::Read is a function that works by indirect effect. It will return
        // "true" on success, and the resulting changes will be found in the _context object.
        UsdMayaPrimReaderContext* context = _context->GetPrimReaderContext();
        if (context == nullptr || !shaderReader.Read(*context)) {
            return MObject();
        }

        MObject shaderObj = shaderReader.GetCreatedObject(*_context, shaderSchema.GetPrim());
        if (shaderObj.isNull()) {
            return MObject();
        }

        for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
            MPlug mayaAttr = shaderReader.GetMayaPlugForUsdAttrName(input.GetFullName(), shaderObj);
            if (mayaAttr.isNull()) {
                continue;
            }

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
                // The exporter can choose to group ancillary nodes in a NodeGraph
                UsdShadeNodeGraph sourceNodeGraph = UsdShadeNodeGraph(source.GetPrim());
                if (!sourceNodeGraph) {
                    continue;
                }

                // Follow through to see if the node graph output is connected:
                const UsdShadeOutput& ngOutput = sourceNodeGraph.GetOutput(sourceOutputName);
                if (!ngOutput
                    || !UsdShadeConnectableAPI::GetConnectedSource(
                        ngOutput, &source, &sourceOutputName, &sourceType)) {
                    continue;
                }

                sourceShaderSchema = UsdShadeShader(source.GetPrim());
                if (!sourceShaderSchema) {
                    continue;
                }
            }

            MPlug srcAttr = _GetSourcePlug(sourceShaderSchema, sourceOutputName);
            if (srcAttr.isNull()) {
                continue;
            }

            UsdMayaUtil::Connect(srcAttr, mayaAttr, false);
        }

        shaderReader.PostConnectSubtree(_context->GetPrimReaderContext());

        return shaderObj;
    }

    using _SdfPathToShaderReaderMap
        = std::unordered_map<SdfPath, UsdMayaShaderReaderSharedPtr, SdfPath::Hash>;

    UsdMayaShadingModeImportContext* _context = nullptr;
    const UsdMayaJobImportArgs&      _jobArguments;
    _SdfPathToShaderReaderMap        _shaderReaderMap;
};

}; // anonymous namespace

DEFINE_SHADING_MODE_IMPORTER_WITH_JOB_ARGUMENTS(
    useRegistry,
    _tokens->NiceName.GetString(),
    _tokens->ImportDescription.GetString(),
    context,
    jobArguments)
{
    UseRegistryShadingModeImporter importer(context, jobArguments);
    return importer.Read();
}

PXR_NAMESPACE_CLOSE_SCOPE
