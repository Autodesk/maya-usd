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
#include "meshWriter.h"

#include <mayaUsd/fileio/primWriter.h>
#include <mayaUsd/fileio/primWriterRegistry.h>
#include <mayaUsd/fileio/translators/translatorMesh.h>
#include <mayaUsd/fileio/utils/adaptor.h>
#include <mayaUsd/fileio/utils/jointWriteUtils.h>
#include <mayaUsd/fileio/utils/meshReadUtils.h>
#include <mayaUsd/fileio/utils/meshWriteUtils.h>
#include <mayaUsd/fileio/utils/writeUtil.h>
#include <mayaUsd/fileio/writeJobContext.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/pointBased.h>
#include <pxr/usd/usdGeom/primvar.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdUtils/pipeline.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MUintArray.h>

#include <set>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(mesh, PxrUsdTranslators_MeshWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(mesh, UsdGeomMesh);

// clang-format off
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((skelJointIndices, "skel:jointIndices"))
    ((skelJointWeights, "skel:jointWeights"))
    ((skelGeomBindTransform, "skel:geomBindTransform"))
);
// clang-format on

PxrUsdTranslators_MeshWriter::PxrUsdTranslators_MeshWriter(
    const MFnDependencyNode& depNodeFn,
    const SdfPath&           usdPath,
    UsdMayaWriteJobContext&  jobCtx)
    : UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx)
{
    MayaUsd::TranslatorMeshWrite meshWriter(depNodeFn, GetUsdStage(), GetUsdPath(), GetDagPath());
    _usdPrim = meshWriter.usdMesh().GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdGeomMesh at path '%s'\n",
            meshWriter.usdMesh().GetPath().GetText())) {
        return;
    }
}

void PxrUsdTranslators_MeshWriter::PostExport() { cleanupPrimvars(); }

void PxrUsdTranslators_MeshWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaPrimWriter::Write(usdTime);

    UsdGeomMesh primSchema(_usdPrim);
    writeMeshAttrs(usdTime, primSchema);
}

bool PxrUsdTranslators_MeshWriter::writeMeshAttrs(
    const UsdTimeCode& usdTime,
    UsdGeomMesh&       primSchema)
{
    MStatus status { MS::kSuccess };

    // Exporting reference object only once
    if (usdTime.IsDefault() && _GetExportArgs().exportReferenceObjects) {
        UsdMayaMeshWriteUtils::exportReferenceMesh(primSchema, GetMayaObject());
    }

    // Write UsdSkel skeletal skinning data first, since this will
    // determine whether we use the "input" or "final" mesh when exporting
    // mesh geometry. This should only be run once at default time.
    if (usdTime.IsDefault()) {
        const TfToken& exportSkin = _GetExportArgs().exportSkin;
        if (exportSkin != UsdMayaJobExportArgsTokens->auto_
            && exportSkin != UsdMayaJobExportArgsTokens->explicit_) {

            _skelInputMesh = MObject();

        } else if (
            exportSkin == UsdMayaJobExportArgsTokens->explicit_
            && !UsdSkelRoot::Find(primSchema.GetPrim())) {

            _skelInputMesh = MObject();

        } else {

            SdfPath skelPath;
            _skelInputMesh = UsdMayaJointUtil::writeSkinningData(
                primSchema,
                GetUsdPath(),
                GetDagPath(),
                skelPath,
                _GetExportArgs().stripNamespaces,
                _GetSparseValueWriter());

            if (!_skelInputMesh.isNull()) {
                // Add all skel primvars to the exclude set.
                // We don't want later processing to stomp on any of our data.
                _excludeColorSets.insert({ _tokens->skelJointIndices,
                                           _tokens->skelJointWeights,
                                           _tokens->skelGeomBindTransform });

                // Mark the bindings for post processing.
                _writeJobCtx.MarkSkelBindings(primSchema.GetPrim().GetPath(), skelPath, exportSkin);
            }
        }
    }

    // This is the mesh that "lives" at the end of this dag node. We should
    // always pull user-editable "sidecar" data like color sets and tags from
    // this mesh.
    MFnMesh finalMesh(GetDagPath(), &status);
    if (!status) {
        TF_RUNTIME_ERROR(
            "Failed to get final mesh at DAG path: %s", GetDagPath().fullPathName().asChar());
        return false;
    }

    // If exporting skinning, then geomMesh and finalMesh will be different
    // meshes. The general rule is to use geomMesh only for geometric data such
    // as vertices, faces, normals, but use finalMesh for UVs, color sets,
    // and user-defined tagging (e.g. subdiv tags).
    MObject geomMeshObj = _skelInputMesh.isNull() ? finalMesh.object() : _skelInputMesh;
    // do not pass these to functions that need access to geomMeshObj!
    // geomMesh.object() returns nil for meshes of type kMeshData.
    MFnMesh geomMesh(geomMeshObj, &status);
    if (!status) {
        TF_RUNTIME_ERROR(
            "Failed to get geom mesh at DAG path: %s", GetDagPath().fullPathName().asChar());
        return false;
    }

    // Return if usdTime does not match if shape is animated.
    if (usdTime.IsDefault() == isMeshAnimated()) {
        // If the shape is animated (based on the check above), only export time
        // samples. If the shape is non-animated, only export at the default
        // time.
        return true;
    }

    // Set mesh attrs ==========
    // Write points
    UsdMayaMeshWriteUtils::writePointsData(geomMesh, primSchema, usdTime, _GetSparseValueWriter());

    // Write faceVertexIndices
    UsdMayaMeshWriteUtils::writeFaceVertexIndicesData(
        geomMesh, primSchema, usdTime, _GetSparseValueWriter());

    // Read subdiv scheme tagging. If not set, we default to defaultMeshScheme
    // flag (this is specified by the job args but defaults to catmullClark).
    TfToken sdScheme = UsdMayaMeshWriteUtils::getSubdivScheme(finalMesh);
    if (sdScheme.IsEmpty()) {
        sdScheme = _GetExportArgs().defaultMeshScheme;
    }
    primSchema.CreateSubdivisionSchemeAttr(VtValue(sdScheme), true);

    if (sdScheme == UsdGeomTokens->none) {
        // Polygonal mesh - export normals.
        bool emitNormals = true; // Default to emitting normals if no tagging.
        UsdMayaMeshReadUtils::getEmitNormalsTag(finalMesh, &emitNormals);
        if (emitNormals) {
            UsdMayaMeshWriteUtils::writeNormalsData(
                geomMesh, primSchema, usdTime, _GetSparseValueWriter());
        }
    } else {
        // Subdivision surface - export subdiv-specific attributes.
        UsdMayaMeshWriteUtils::writeSubdivInterpBound(
            finalMesh, primSchema, _GetSparseValueWriter());

        UsdMayaMeshWriteUtils::writeSubdivFVLinearInterpolation(
            finalMesh, primSchema, _GetSparseValueWriter());

        UsdMayaMeshWriteUtils::assignSubDivTagsToUSDPrim(
            finalMesh, primSchema, _GetSparseValueWriter());
    }

    // Holes - we treat InvisibleFaces as holes
    UsdMayaMeshWriteUtils::writeInvisibleFacesData(finalMesh, primSchema, _GetSparseValueWriter());

    // == Write UVSets as Vec2f Primvars
    if (_GetExportArgs().exportMeshUVs) {
        UsdMayaMeshWriteUtils::writeUVSetsAsVec2fPrimvars(
            finalMesh, primSchema, usdTime, _GetSparseValueWriter());
    }

    // == Gather ColorSets
    std::vector<std::string> colorSetNames;
    if (_GetExportArgs().exportColorSets) {
        MStringArray mayaColorSetNames;
        status = finalMesh.getColorSetNames(mayaColorSetNames);
        colorSetNames.reserve(mayaColorSetNames.length());
        for (unsigned int i = 0; i < mayaColorSetNames.length(); i++) {
            colorSetNames.emplace_back(mayaColorSetNames[i].asChar());
        }
    }

    std::set<std::string> colorSetNamesSet(colorSetNames.begin(), colorSetNames.end());

    VtArray<GfVec3f> shadersRGBData;
    VtArray<float>   shadersAlphaData;
    TfToken          shadersInterpolation;
    VtArray<int>     shadersAssignmentIndices;

    // If we're exporting displayColor or we have color sets, gather colors and
    // opacities from the shaders assigned to the mesh and/or its faces.
    // If we find a displayColor color set, the shader colors and opacities
    // will be used to fill in unauthored/unpainted faces in the color set.
    if (_GetExportArgs().exportDisplayColor || !colorSetNames.empty()) {
        UsdMayaUtil::GetLinearShaderColor(
            finalMesh,
            &shadersRGBData,
            &shadersAlphaData,
            &shadersInterpolation,
            &shadersAssignmentIndices);
    }

    for (const std::string& colorSetName : colorSetNames) {

        if (_excludeColorSets.count(colorSetName) > 0)
            continue;

        bool isDisplayColor = false;

        if (colorSetName == UsdMayaMeshPrimvarTokens->DisplayColorColorSetName.GetString()) {
            if (!_GetExportArgs().exportDisplayColor) {
                continue;
            }
            isDisplayColor = true;
        }

        if (colorSetName == UsdMayaMeshPrimvarTokens->DisplayOpacityColorSetName.GetString()) {
            TF_WARN(
                "Mesh \"%s\" has a color set named \"%s\", "
                "which is a reserved Primvar name in USD. Skipping...",
                finalMesh.fullPathName().asChar(),
                UsdMayaMeshPrimvarTokens->DisplayOpacityColorSetName.GetText());
            continue;
        }

        VtArray<GfVec3f>              RGBData;
        VtArray<float>                AlphaData;
        TfToken                       interpolation;
        VtArray<int>                  assignmentIndices;
        MFnMesh::MColorRepresentation colorSetRep;
        bool                          clamped = false;

        if (!UsdMayaMeshWriteUtils::getMeshColorSetData(
                finalMesh,
                MString(colorSetName.c_str()),
                isDisplayColor,
                shadersRGBData,
                shadersAlphaData,
                shadersAssignmentIndices,
                &RGBData,
                &AlphaData,
                &interpolation,
                &assignmentIndices,
                &colorSetRep,
                &clamped)) {
            TF_WARN(
                "Unable to retrieve colorSet data: %s on mesh: %s. "
                "Skipping...",
                colorSetName.c_str(),
                finalMesh.fullPathName().asChar());
            continue;
        }

        if (isDisplayColor) {
            // We tag the resulting displayColor/displayOpacity primvar as
            // authored to make sure we reconstruct the color set on import.
            UsdMayaMeshWriteUtils::addDisplayPrimvars(
                primSchema,
                usdTime,
                colorSetRep,
                RGBData,
                AlphaData,
                interpolation,
                assignmentIndices,
                clamped,
                true,
                _GetSparseValueWriter());
        } else {
            const std::string sanitizedName = UsdMayaUtil::SanitizeColorSetName(colorSetName);
            // if our sanitized name is different than our current one and the
            // sanitized name already exists, it means 2 things are trying to
            // write to the same primvar.  warn and continue.
            if (colorSetName != sanitizedName && colorSetNamesSet.count(sanitizedName) > 0) {
                TF_WARN(
                    "Skipping colorSet '%s' as the colorSet '%s' exists as "
                    "well.",
                    colorSetName.c_str(),
                    sanitizedName.c_str());
                continue;
            }

            TfToken colorSetNameToken = TfToken(sanitizedName);
            if (colorSetRep == MFnMesh::kAlpha) {
                UsdMayaMeshWriteUtils::createAlphaPrimVar(
                    primSchema,
                    colorSetNameToken,
                    usdTime,
                    AlphaData,
                    interpolation,
                    assignmentIndices,
                    clamped,
                    _GetSparseValueWriter());
            } else if (colorSetRep == MFnMesh::kRGB) {
                UsdMayaMeshWriteUtils::createRGBPrimVar(
                    primSchema,
                    colorSetNameToken,
                    usdTime,
                    RGBData,
                    interpolation,
                    assignmentIndices,
                    clamped,
                    _GetSparseValueWriter());
            } else if (colorSetRep == MFnMesh::kRGBA) {
                UsdMayaMeshWriteUtils::createRGBAPrimVar(
                    primSchema,
                    colorSetNameToken,
                    usdTime,
                    RGBData,
                    AlphaData,
                    interpolation,
                    assignmentIndices,
                    clamped,
                    _GetSparseValueWriter());
            }
        }
    }

    // UsdMayaMeshWriteUtils::addDisplayPrimvars() will only author displayColor and displayOpacity
    // if no authored opinions exist, so the code below only has an effect if
    // we did NOT find a displayColor color set above.
    if (_GetExportArgs().exportDisplayColor) {
        // Using the shader default values (an alpha of zero, in particular)
        // results in Gprims rendering the same way in usdview as they do in
        // Maya (i.e. unassigned components are invisible).
        //
        // Since these colors come from the shaders and not a colorset, we are
        // not adding the clamp attribute as custom data. We also don't need to
        // reconstruct a color set from them on import since they originated
        // from the bound shader(s), so the authored flag is set to false.
        UsdMayaMeshWriteUtils::addDisplayPrimvars(
            primSchema,
            usdTime,
            MFnMesh::kRGBA,
            shadersRGBData,
            shadersAlphaData,
            shadersInterpolation,
            shadersAssignmentIndices,
            false,
            false,
            _GetSparseValueWriter());
    }

    return true;
}

bool PxrUsdTranslators_MeshWriter::ExportsGprims() const { return true; }

bool PxrUsdTranslators_MeshWriter::isMeshAnimated() const
{
    // Note that _HasAnimCurves() as computed by UsdMayaTransformWriter is
    // whether the finalMesh is animated.
    return _skelInputMesh.isNull() ? _HasAnimCurves() : false;
}

void PxrUsdTranslators_MeshWriter::cleanupPrimvars()
{
    if (!isMeshAnimated()) {
        // Based on how setPrimvar() works, the cleanup phase doesn't apply to
        // non-animated meshes.
        return;
    }

    // On animated meshes, we forced an extra value (the "unassigned" or
    // "unauthored" value) into index 0 of any indexed primvar's values array.
    // If the indexed primvar doesn't need the unassigned value (because all
    // of the indices are assigned), then we can remove the unassigned value
    // and shift all the indices down.
    const UsdGeomMesh primSchema(GetUsdPrim());
    for (const UsdGeomPrimvar& primvar : primSchema.GetPrimvars()) {
        if (!primvar) {
            continue;
        }

        // Cleanup phase applies only to indexed primvars.
        // Unindexed primvars were written directly without modification.
        if (!primvar.IsIndexed()) {
            continue;
        }

        // If the unauthoredValueIndex is 0, that means we purposefully set it
        // to indicate that at least one time sample has unauthored values.
        const int unauthoredValueIndex = primvar.GetUnauthoredValuesIndex();
        if (unauthoredValueIndex == 0) {
            continue;
        }

        // If the unauthoredValueIndex wasn't 0 above, it must be -1 (the
        // fallback value in USD).
        if (!TF_VERIFY(unauthoredValueIndex == -1)) {
            return;
        }

        // Since the unauthoredValueIndex is -1, we never explicitly set it,
        // meaning that none of the samples contain an unassigned value.
        // Since we authored the unassigned value as index 0 in each primvar,
        // we can eliminate it now from all time samples.
        if (const UsdAttribute attr = primvar.GetAttr()) {
            VtValue val;
            if (attr.Get(&val, UsdTimeCode::Default())) {
                const VtValue newVal = UsdMayaUtil::popFirstValue(val);
                if (!newVal.IsEmpty()) {
                    attr.Set(newVal, UsdTimeCode::Default());
                }
            }
            std::vector<double> timeSamples;
            if (attr.GetTimeSamples(&timeSamples)) {
                for (const double& t : timeSamples) {
                    if (attr.Get(&val, t)) {
                        const VtValue newVal = UsdMayaUtil::popFirstValue(val);
                        if (!newVal.IsEmpty()) {
                            attr.Set(newVal, t);
                        }
                    }
                }
            }
        }

        // We then need to shift all the indices down one to account for index
        // 0 being eliminated.
        if (const UsdAttribute attr = primvar.GetIndicesAttr()) {
            VtIntArray val;
            if (attr.Get(&val, UsdTimeCode::Default())) {
                attr.Set(UsdMayaUtil::shiftIndices(val, -1), UsdTimeCode::Default());
            }
            std::vector<double> timeSamples;
            if (attr.GetTimeSamples(&timeSamples)) {
                for (const double& t : timeSamples) {
                    if (attr.Get(&val, t)) {
                        attr.Set(UsdMayaUtil::shiftIndices(val, -1), t);
                    }
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
