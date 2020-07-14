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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((DefaultShaderOutputName, "out"))
    ((MayaShaderOutputName, "outColor"))
);

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

class UseRegistryShadingModeImporter {
public:
    UseRegistryShadingModeImporter() = delete;
    UseRegistryShadingModeImporter(UsdMayaShadingModeImportContext* context)
        : _context(context)
    {
    }

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

        auto surfaceReader = _GetOrCreateShaderReader(surfaceShader);
        if (!surfaceReader) {
            return MObject();
        }
        MObject surfaceShaderObj = surfaceReader->GetMayaObject();

        if (surfaceShaderObj.isNull()) {
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

        const TfToken surfaceShaderPlugName = _context->GetSurfaceShaderPlugName();
        if (!surfaceShaderPlugName.IsEmpty() && !surfaceShaderObj.isNull()) {
            MFnDependencyNode depNodeFn(surfaceShaderObj, &status);
            if (status != MS::kSuccess) {
                return MObject();
            }

            MPlug shaderOutputPlug
                = depNodeFn.findPlug(_tokens->MayaShaderOutputName.GetText(), &status);
            if (status != MS::kSuccess || shaderOutputPlug.isNull()) {
                return MObject();
            }

            MPlug seInputPlug = fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
            CHECK_MSTATUS_AND_RETURN(status, MObject());

            UsdMayaUtil::Connect(
                shaderOutputPlug,
                seInputPlug,
                /* clearDstPlug = */ true);
        }

        return shadingEngine;
    }

private:
    /// Gets a shader reader for \p usdNode
    ///
    /// If no shader writer can be found for the Maya node or if the node
    /// otherwise should not be authored, an empty pointer is returned.
    ///
    UsdMayaShaderReaderSharedPtr _GetOrCreateShaderReader(const UsdShadeShader& shaderSchema)
    {
        const SdfPath& shaderPath(shaderSchema.GetPath());
        const auto     iter = _shaderReaderMap.find(shaderPath);
        if (iter != _shaderReaderMap.end()) {
            // We've already created a shader reader for this node, so just
            // return it.
            return iter->second;
        }

        // No shader reader exists for this node yet, so create one.
        UsdMayaShaderReaderSharedPtr shaderReader;

        TfToken shaderId;
        shaderSchema.GetIdAttr().Get(&shaderId);

        if (UsdMayaShaderReaderRegistry::ReaderFactoryFn factoryFn
            = UsdMayaShaderReaderRegistry::Find(shaderId.GetText())) {

            UsdMayaJobImportArgs defaultJobArgs = UsdMayaJobImportArgs::CreateFromDictionary(
                UsdMayaJobImportArgs::GetDefaultDictionary());
            UsdPrim shaderPrim = shaderSchema.GetPrim();
            UsdMayaPrimReaderArgs args(shaderPrim, defaultJobArgs);

            shaderReader = std::dynamic_pointer_cast<UsdMayaShaderReader>(factoryFn(args));

            if (!_PopulateShaderReader(shaderSchema, *shaderReader)) {
                // Read failed. Invalidate the reader.
                shaderReader = nullptr;
            }
        }

        // Store the shader writer pointer whether we succeeded or not so
        // that we don't repeatedly attempt and fail to create it for the
        // same node.
        _shaderReaderMap[shaderPath] = shaderReader;

        return shaderReader;
    }

    bool
    _PopulateShaderReader(const UsdShadeShader& shaderSchema, UsdMayaShaderReader& shaderReader)
    {
        bool readResult = shaderReader.Read(_context->GetPrimReaderContext());
        if (!readResult) {
            return false;
        }

        MObject shaderObj = shaderReader.GetMayaObject();
        if (shaderObj.isNull()) {
            return false;
        }

        _context->AddCreatedObject(shaderSchema.GetPrim(), shaderObj);

        MStatus           status;
        MFnDependencyNode depFn(shaderObj, &status);
        if (status != MS::kSuccess) {
            return false;
        }

        for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
            auto mayaAttrName = shaderReader.GetMayaNameForUsdAttrName(input.GetFullName());
            if (mayaAttrName == TfToken()) {
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

            auto    sourceReader = _GetOrCreateShaderReader(sourceShaderSchema);
            MObject sourceObj = sourceReader->GetMayaObject();

            MFnDependencyNode sourceDepFn(sourceObj, &status);
            if (status != MS::kSuccess) {
                continue;
            }

            sourceOutputName
                = TfToken(TfStringPrintf(
                              "%s%s", UsdShadeTokens->outputs.GetText(), sourceOutputName.GetText())
                              .c_str());
            auto  srcAttrName = sourceReader->GetMayaNameForUsdAttrName(sourceOutputName);
            MPlug srcAttr = sourceDepFn.findPlug(srcAttrName.GetText());
            if (srcAttr.isArray()) {
                const unsigned int numElements = srcAttr.evaluateNumElements();
                if (numElements > 0u) {
                    if (numElements > 1u) {
                        TF_WARN(
                            "Array with multiple elements encountered at '%s'. "
                            "Currently, only arrays with a single element are "
                            "supported. Not connecting attribute.",
                            srcAttr.name().asChar());
                        continue;
                    }

                    srcAttr = srcAttr[0];
                }
            }

            UsdMayaUtil::Connect(srcAttr, mayaAttr, false);
        }
        return true;
    }

    using _SdfPathToShaderReaderMap
        = std::unordered_map<SdfPath, UsdMayaShaderReaderSharedPtr, SdfPath::Hash>;

    UsdMayaShadingModeImportContext* _context = nullptr;
    _SdfPathToShaderReaderMap        _shaderReaderMap;
};

}; // anonymous namespace

DEFINE_SHADING_MODE_IMPORTER(useRegistry, context)
{
    UseRegistryShadingModeImporter importer(context);
    return importer.Read();
}

PXR_NAMESPACE_CLOSE_SCOPE
