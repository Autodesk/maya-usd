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
// Modifications copyright (C) 2020 Autodesk
//
#include "meshUtil.h"
#include "roundTripUtil.h"
#include "readUtil.h"
#include "adaptor.h"

#include "../../base/debugCodes.h"
#include "../../utils/util.h"
#include "../../utils/colorSpace.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdUtils/pipeline.h"

#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPartition.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MUintArray.h>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(UsdMayaMeshColorSetTokens,
    PXRUSDMAYA_MESH_COLOR_SET_TOKENS);

// These tokens are supported Maya attributes used for Mesh surfaces
TF_DEFINE_PRIVATE_TOKENS(
    _meshTokens,

    // we capitalize this because it doesn't correspond to an actual attribute
    (USD_EmitNormals)

    // This is a value for face varying interpolate boundary from OpenSubdiv 2
    // that we translate to face varying linear interpolation for OpenSubdiv 3.
    (alwaysSharp)

    // This token is deprecated as it is from OpenSubdiv 2 and the USD
    // schema now conforms to OpenSubdiv 3, but we continue to look for it
    // and translate to the equivalent new value for backwards compatibility.
    (USD_faceVaryingInterpolateBoundary)
);

PXRUSDMAYA_REGISTER_ADAPTOR_ATTRIBUTE_ALIAS(
        UsdGeomTokens->subdivisionScheme,
        "USD_subdivisionScheme");
PXRUSDMAYA_REGISTER_ADAPTOR_ATTRIBUTE_ALIAS(
        UsdGeomTokens->interpolateBoundary,
        "USD_interpolateBoundary");
PXRUSDMAYA_REGISTER_ADAPTOR_ATTRIBUTE_ALIAS(
        UsdGeomTokens->faceVaryingLinearInterpolation,
        "USD_faceVaryingLinearInterpolation");

namespace
{
    bool addCreaseSet( const std::string &rootName,
                       double creaseLevel,
                       MSelectionList &componentList,
                       MStatus *statusOK )
    {
        // Crease Set functionality is native to Maya, but undocumented and not
        // directly supported in the API. The below implementation is derived from
        // the editor code in the maya distro at:
        //
        // .../lib/python2.7/site-packages/maya/app/general/creaseSetEditor.py

        MObject creasePartitionObj;
        *statusOK = UsdMayaUtil::GetMObjectByName(":creasePartition",
                                                     creasePartitionObj);

        if (creasePartitionObj.isNull()) {
            statusOK->clear();

            // There is no documented way to create a shared node through the C++ API
            const std::string partitionName = MGlobal::executeCommandStringResult(
                "createNode \"partition\" -shared -name \":creasePartition\"").asChar();

            *statusOK = UsdMayaUtil::GetMObjectByName(partitionName,
                                                         creasePartitionObj);
            if (!*statusOK) {
                return false;
            }
        }

        MFnPartition creasePartition( creasePartitionObj, statusOK );
        if (!*statusOK) return false;

        std::string creaseSetname =
            TfStringPrintf("%s_creaseSet#",rootName.c_str());

        MFnDependencyNode creaseSetFn;
        MObject creaseSetObj =
            creaseSetFn.create("creaseSet", creaseSetname.c_str(), statusOK );
        if (!*statusOK) return false;

        MPlug levelPlug = creaseSetFn.findPlug("creaseLevel",false, statusOK);
        if (!*statusOK) return false;

        *statusOK = levelPlug.setValue(creaseLevel);
        if (!*statusOK) return false;

        *statusOK = creasePartition.addMember(creaseSetObj);
        if (!*statusOK) return false;

        MFnSet creaseSet( creaseSetObj, statusOK );
        if (!*statusOK) return false;

        *statusOK = creaseSet.addMembers( componentList );
        if (!*statusOK) return false;

        return true;
    }

    MIntArray
    getMayaFaceVertexAssignmentIds( const MFnMesh& meshFn,
                                    const TfToken& interpolation,
                                    const VtIntArray& assignmentIndices,
                                    const int unauthoredValuesIndex)
    {
        MIntArray valueIds(meshFn.numFaceVertices(), -1);

        MItMeshFaceVertex itFV(meshFn.object());
        unsigned int fvi = 0;
        for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
            int valueId = 0;
            if (interpolation == UsdGeomTokens->constant) {
                valueId = 0;
            } else if (interpolation == UsdGeomTokens->uniform) {
                valueId = itFV.faceId();
            } else if (interpolation == UsdGeomTokens->vertex) {
                valueId = itFV.vertId();
            } else if (interpolation == UsdGeomTokens->faceVarying) {
                valueId = fvi;
            }

            if (static_cast<size_t>(valueId) < assignmentIndices.size()) {
                // The data is indexed, so consult the indices array for the
                // correct index into the data.
                valueId = assignmentIndices[valueId];

                if (valueId == unauthoredValuesIndex) {
                    // This component had no authored value, so leave it unassigned.
                    continue;
                }
            }

            valueIds[fvi] = valueId;
        }

        return valueIds;
    }

    bool 
    assignUVSetPrimvarToMesh(const UsdGeomPrimvar& primvar, MFnMesh& meshFn)
    {
        const TfToken& primvarName = primvar.GetPrimvarName();

        // Get the raw data before applying any indexing.
        VtVec2fArray uvValues;
        if (!primvar.Get(&uvValues) || uvValues.empty()) {
            TF_WARN("Could not read UV values from primvar '%s' on mesh: %s",
                    primvarName.GetText(),
                    primvar.GetAttr().GetPrimPath().GetText());
            return false;
        }

        // This is the number of UV values assuming the primvar is NOT indexed.
        VtIntArray assignmentIndices;
        if (primvar.GetIndices(&assignmentIndices)) {
            // The primvar IS indexed, so the indices array is what determines the
            // number of UV values.
            int unauthoredValuesIndex = primvar.GetUnauthoredValuesIndex();

            // Replace any index equal to unauthoredValuesIndex with -1.
            if (unauthoredValuesIndex != -1) {
                for (int& index : assignmentIndices) {
                    if (index == unauthoredValuesIndex) {
                        index = -1;
                    }
                }
            }

            // Furthermore, if unauthoredValuesIndex is valid for uvValues, then
            // remove it from uvValues and shift the indices (we don't want to
            // import the unauthored value into Maya, where it has no meaning).
            if (unauthoredValuesIndex >= 0 &&
                    static_cast<size_t>(unauthoredValuesIndex) < uvValues.size()) {
                // This moves [unauthoredValuesIndex + 1, end) to
                // [unauthoredValuesIndex, end - 1), erasing the
                // unauthoredValuesIndex.
                std::move(
                        uvValues.begin() + unauthoredValuesIndex + 1,
                        uvValues.end(),
                        uvValues.begin() + unauthoredValuesIndex);
                uvValues.pop_back();

                for (int& index : assignmentIndices) {
                    if (index > unauthoredValuesIndex) {
                        index = index - 1;
                    }
                }
            }
        }

        // Go through the UV data and add the U and V values to separate
        // MFloatArrays.
        MFloatArray uCoords;
        MFloatArray vCoords;
        for (const GfVec2f& v : uvValues) {
            uCoords.append(v[0]);
            vCoords.append(v[1]);
        }

        MStatus status{MS::kSuccess};
        MString uvSetName(primvarName.GetText());
        if (primvarName == UsdUtilsGetPrimaryUVSetName()) {
            // We assume that the primary USD UV set maps to Maya's default 'map1'
            // set which always exists, so we shouldn't try to create it.
            uvSetName = "map1";
        } else {
            status = meshFn.createUVSet(uvSetName);
            if (status != MS::kSuccess) {
                TF_WARN("Unable to create UV set '%s' for mesh: %s",
                        uvSetName.asChar(),
                        meshFn.fullPathName().asChar());
                return false;
            }
        }

        // The following two lines should have no effect on user-visible state but
        // prevent a Maya crash in MFnMesh.setUVs after creating a crease set.
        // XXX this workaround is needed pending a fix by Autodesk.
        MString currentSet = meshFn.currentUVSetName();
        meshFn.setCurrentUVSetName(currentSet);

        // Create UVs on the mesh from the values we collected out of the primvar.
        // We'll assign mesh components to these values below.
        status = meshFn.setUVs(uCoords, vCoords, &uvSetName);
        if (status != MS::kSuccess) {
            TF_WARN("Unable to set UV data on UV set '%s' for mesh: %s",
                    uvSetName.asChar(),
                    meshFn.fullPathName().asChar());
            return false;
        }

        const TfToken& interpolation = primvar.GetInterpolation();

        // Build an array of value assignments for each face vertex in the mesh.
        // Any assignments left as -1 will not be assigned a value.
        MIntArray uvIds = getMayaFaceVertexAssignmentIds(meshFn,
                                                          interpolation,
                                                          assignmentIndices,
                                                          -1);

        MIntArray vertexCounts;
        MIntArray vertexList;
        status = meshFn.getVertices(vertexCounts, vertexList);
        if (status != MS::kSuccess) {
            TF_WARN("Could not get vertex counts for UV set '%s' on mesh: %s",
                    uvSetName.asChar(),
                    meshFn.fullPathName().asChar());
            return false;
        }

        status = meshFn.assignUVs(vertexCounts, uvIds, &uvSetName);
        if (status != MS::kSuccess) {
            TF_WARN("Could not assign UV values to UV set '%s' on mesh: %s",
                    uvSetName.asChar(),
                    meshFn.fullPathName().asChar());
            return false;
        }

        return true;
    }

    bool 
    assignColorSetPrimvarToMesh(const UsdGeomMesh& mesh,
                                const UsdGeomPrimvar& primvar,
                                MFnMesh& meshFn)
    {

        const TfToken& primvarName = primvar.GetPrimvarName();
        const SdfValueTypeName& typeName = primvar.GetTypeName();

        MString colorSetName(primvarName.GetText());

        // If the primvar is displayOpacity and it is a FloatArray, check if
        // displayColor is authored. If not, we'll import this 'displayOpacity'
        // primvar as a 'displayColor' color set. This supports cases where the
        // user created a single channel value for displayColor.
        // Note that if BOTH displayColor and displayOpacity are authored, they will
        // be imported as separate color sets. We do not attempt to combine them
        // into a single color set.
        if (primvarName == UsdMayaMeshColorSetTokens->DisplayOpacityColorSetName &&
                typeName == SdfValueTypeNames->FloatArray) {
            if (!UsdMayaRoundTripUtil::IsAttributeUserAuthored(mesh.GetDisplayColorPrimvar())) {
                colorSetName = UsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetText();
            }
        }

        // We'll need to convert colors from linear to display if this color set is
        // for display colors.
        const bool isDisplayColor =
            (colorSetName == UsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetText());

        // Get the raw data before applying any indexing. We'll only populate one
        // of these arrays based on the primvar's typeName, and we'll also set the
        // color representation so we know which array to use later.
        VtFloatArray alphaArray;
        VtVec3fArray rgbArray;
        VtVec4fArray rgbaArray;
        MFnMesh::MColorRepresentation colorRep;
        size_t numValues = 0;

        MStatus status = MS::kSuccess;

        if (typeName == SdfValueTypeNames->FloatArray) {
            colorRep = MFnMesh::kAlpha;
            if (!primvar.Get(&alphaArray) || alphaArray.empty()) {
                status = MS::kFailure;
            } else {
                numValues = alphaArray.size();
            }
        } else if (typeName == SdfValueTypeNames->Float3Array ||
                   typeName == SdfValueTypeNames->Color3fArray) {
            colorRep = MFnMesh::kRGB;
            if (!primvar.Get(&rgbArray) || rgbArray.empty()) {
                status = MS::kFailure;
            } else {
                numValues = rgbArray.size();
            }
        } else if (typeName == SdfValueTypeNames->Float4Array ||
                   typeName == SdfValueTypeNames->Color4fArray) {
            colorRep = MFnMesh::kRGBA;
            if (!primvar.Get(&rgbaArray) || rgbaArray.empty()) {
                status = MS::kFailure;
            } else {
                numValues = rgbaArray.size();
            }
        } else {
            TF_WARN("Unsupported color set primvar type '%s' for primvar '%s' on "
                    "mesh: %s",
                    typeName.GetAsToken().GetText(),
                    primvarName.GetText(),
                    primvar.GetAttr().GetPrimPath().GetText());
            return false;
        }

        if (status != MS::kSuccess || numValues == 0) {
            TF_WARN("Could not read color set values from primvar '%s' on mesh: %s",
                    primvarName.GetText(),
                    primvar.GetAttr().GetPrimPath().GetText());
            return false;
        }

        VtIntArray assignmentIndices;
        int unauthoredValuesIndex = -1;
        if (primvar.GetIndices(&assignmentIndices)) {
            // The primvar IS indexed, so the indices array is what determines the
            // number of color values.
            numValues = assignmentIndices.size();
            unauthoredValuesIndex = primvar.GetUnauthoredValuesIndex();
        }

        // Go through the color data and translate the values into MColors in the
        // colorArray, taking into consideration that indexed data may have been
        // authored sparsely. If the assignmentIndices array is empty then the data
        // is NOT indexed.
        // Note that with indexed data, the data is added to the arrays in ascending
        // component ID order according to the primvar's interpolation (ascending
        // face ID for uniform interpolation, ascending vertex ID for vertex
        // interpolation, etc.). This ordering may be different from the way the
        // values are ordered in the primvar. Because of this, we recycle the
        // assignmentIndices array as we go to store the new mapping from component
        // index to color index.
        MColorArray colorArray;
        for (size_t i = 0; i < numValues; ++i) {
            int valueIndex = i;

            if (i < assignmentIndices.size()) {
                // The data is indexed, so consult the indices array for the
                // correct index into the data.
                valueIndex = assignmentIndices[i];

                if (valueIndex == unauthoredValuesIndex) {
                    // This component is unauthored, so just update the
                    // mapping in assignmentIndices and then skip the value.
                    // We don't actually use the value at the unassigned index.
                    assignmentIndices[i] = -1;
                    continue;
                }

                // We'll be appending a new value, so the current length of the
                // array gives us the new value's index.
                assignmentIndices[i] = colorArray.length();
            }

            GfVec4f colorValue(1.0);

            switch(colorRep) {
                case MFnMesh::kAlpha:
                    colorValue[3] = alphaArray[valueIndex];
                    break;
                case MFnMesh::kRGB:
                    colorValue[0] = rgbArray[valueIndex][0];
                    colorValue[1] = rgbArray[valueIndex][1];
                    colorValue[2] = rgbArray[valueIndex][2];
                    break;
                case MFnMesh::kRGBA:
                    colorValue[0] = rgbaArray[valueIndex][0];
                    colorValue[1] = rgbaArray[valueIndex][1];
                    colorValue[2] = rgbaArray[valueIndex][2];
                    colorValue[3] = rgbaArray[valueIndex][3];
                    break;
                default:
                    break;
            }

            if (isDisplayColor) {
                colorValue = UsdMayaColorSpace::ConvertLinearToMaya(colorValue);
            }

            MColor mColor(colorValue[0], colorValue[1], colorValue[2], colorValue[3]);
            colorArray.append(mColor);
        }

        // colorArray now stores all of the values and any unassigned components
        // have had their indices set to -1, so update the unauthored values index.
        unauthoredValuesIndex = -1;

        const bool clamped = UsdMayaRoundTripUtil::IsPrimvarClamped(primvar);

        status = meshFn.createColorSet(colorSetName, nullptr, clamped, colorRep);
        if (status != MS::kSuccess) {
            TF_WARN("Unable to create color set '%s' for mesh: %s",
                    colorSetName.asChar(),
                    meshFn.fullPathName().asChar());
            return false;
        }

        // Create colors on the mesh from the values we collected out of the
        // primvar. We'll assign mesh components to these values below.
        status = meshFn.setColors(colorArray, &colorSetName, colorRep);
        if (status != MS::kSuccess) {
            TF_WARN("Unable to set color data on color set '%s' for mesh: %s",
                    colorSetName.asChar(),
                    meshFn.fullPathName().asChar());
            return false;
        }

        const TfToken& interpolation = primvar.GetInterpolation();

        // Build an array of value assignments for each face vertex in the mesh.
        // Any assignments left as -1 will not be assigned a value.
        MIntArray colorIds = getMayaFaceVertexAssignmentIds(meshFn,
                                                             interpolation,
                                                             assignmentIndices,
                                                             unauthoredValuesIndex);

        status = meshFn.assignColors(colorIds, &colorSetName);
        if (status != MS::kSuccess) {
            TF_WARN("Could not assign color values to color set '%s' on mesh: %s",
                    colorSetName.asChar(),
                    meshFn.fullPathName().asChar());
            return false;
        }

        // we only visualize the colorset by default if it is "displayColor".
        // this is a limitation and affects user experience. This needs further review. HS, 1-Nov-2019
        MStringArray colorSetNames;
        if (meshFn.getColorSetNames(colorSetNames) == MS::kSuccess) 
        {
            for (unsigned int i = 0u; i < colorSetNames.length(); ++i) 
            {
                const MString colorSetName = colorSetNames[i];

                if (std::string(colorSetName.asChar())
                        == UsdMayaMeshColorSetTokens->DisplayColorColorSetName.GetString()) 
                {
                    const auto csRep = meshFn.getColorRepresentation(colorSetName);

                    if (csRep == MFnMesh::kRGB || csRep == MFnMesh::kRGBA) 
                    {
                        meshFn.setCurrentColorSetName(colorSetName);
                    }
                    break;
                }
            }

            MPlug plg = meshFn.findPlug("displayColors");
            if (!plg.isNull()) {
                plg.setBool(true);
            }
        }

        return true;
    }

    bool 
    assignConstantPrimvarToMesh(const UsdGeomPrimvar& primvar, MFnMesh& meshFn)
    {
        const TfToken& interpolation = primvar.GetInterpolation();
        if (interpolation != UsdGeomTokens->constant) {
            return false;
        }

        const TfToken& name = primvar.GetBaseName();
        const SdfValueTypeName& typeName = primvar.GetTypeName();
        const SdfVariability& variability = SdfVariabilityUniform;

        MObject attrObj =
            UsdMayaReadUtil::FindOrCreateMayaAttr(
                typeName,
                variability,
                meshFn,
                name.GetText());
        if (attrObj.isNull()) {
            return false;
        }

        VtValue primvarData;
        primvar.Get(&primvarData);

        MStatus status{MS::kSuccess};
        MPlug plug = meshFn.findPlug(
            name.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
        if (!status || plug.isNull()) {
            return false;
        }

        return UsdMayaReadUtil::SetMayaAttr(plug, primvarData);
    }
}

// This can be customized for specific pipelines.
bool
UsdMayaMeshUtil::GetEmitNormalsTag(const MFnMesh& mesh, bool* value)
{
    MPlug plug = mesh.findPlug(MString(_meshTokens->USD_EmitNormals.GetText()));
    if (!plug.isNull()) {
        *value = plug.asBool();
        return true;
    }

    return false;
}

void
UsdMayaMeshUtil::SetEmitNormalsTag(
        MFnMesh& meshFn,
        const bool emitNormals)
{
    MStatus status{MS::kSuccess};
    MFnNumericAttribute nAttr;
    MObject attr = nAttr.create(_meshTokens->USD_EmitNormals.GetText(),
                                "", MFnNumericData::kBoolean, 0, &status);
    if (status == MS::kSuccess) {
        meshFn.addAttribute(attr);
        MPlug plug = meshFn.findPlug(attr);
        if (!plug.isNull()) {
            plug.setBool(emitNormals);
        }
    }
}

bool
UsdMayaMeshUtil::GetMeshNormals(
        const MFnMesh& mesh,
        VtArray<GfVec3f>* normalsArray,
        TfToken* interpolation)
{
    MStatus status{MS::kSuccess};

    // Sanity check first to make sure we can get this mesh's normals.
    int numNormals = mesh.numNormals(&status);
    if (status != MS::kSuccess || numNormals == 0) {
        return false;
    }

    // Using itFV.getNormal() does not always give us the right answer, so
    // instead we have to use itFV.normalId() and use that to index into the
    // normals.
    MFloatVectorArray mayaNormals;
    status = mesh.getNormals(mayaNormals);
    if (status != MS::kSuccess) {
        return false;
    }

    const unsigned int numFaceVertices = mesh.numFaceVertices(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    normalsArray->resize(numFaceVertices);
    *interpolation = UsdGeomTokens->faceVarying;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        int normalId = itFV.normalId();
        if (normalId < 0 ||
                static_cast<size_t>(normalId) >= mayaNormals.length()) {
            return false;
        }

        MFloatVector normal = mayaNormals[normalId];
        (*normalsArray)[fvi][0] = normal[0];
        (*normalsArray)[fvi][1] = normal[1];
        (*normalsArray)[fvi][2] = normal[2];
    }

    return true;
}

// This can be customized for specific pipelines.
// We first look for the USD string attribute, and if not present we look for
// the RenderMan for Maya int attribute.
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken
UsdMayaMeshUtil::GetSubdivScheme(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken schemeToken;
    UsdMayaAdaptor(mesh.object())
            .GetSchemaOrInheritedSchema<UsdGeomMesh>()
            .GetAttribute(UsdGeomTokens->subdivisionScheme)
            .Get<TfToken>(&schemeToken);

    // Fall back to the RenderMan for Maya attribute.
    if (schemeToken.IsEmpty()) {
        MPlug plug = mesh.findPlug(MString("rman__torattr___subdivScheme"));
        if (!plug.isNull()) {
            switch (plug.asInt()) {
                case 0:
                    schemeToken = UsdGeomTokens->catmullClark;
                    break;
                case 1:
                    schemeToken = UsdGeomTokens->loop;
                    break;
                default:
                    break;
            }
        }
    }

    if (schemeToken.IsEmpty()) {
        return TfToken();
    } else if (schemeToken != UsdGeomTokens->none &&
               schemeToken != UsdGeomTokens->catmullClark &&
               schemeToken != UsdGeomTokens->loop &&
               schemeToken != UsdGeomTokens->bilinear) {
        TF_RUNTIME_ERROR(
                "Unsupported subdivision scheme: %s on mesh: %s",
                schemeToken.GetText(), mesh.fullPathName().asChar());
        return TfToken();
    }

    return schemeToken;
}

// This can be customized for specific pipelines.
// We first look for the USD string attribute, and if not present we look for
// the RenderMan for Maya int attribute.
// XXX Maybe we should come up with a OSD centric nomenclature ??
TfToken UsdMayaMeshUtil::GetSubdivInterpBoundary(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken interpBoundaryToken;
    UsdMayaAdaptor(mesh.object())
            .GetSchemaOrInheritedSchema<UsdGeomMesh>()
            .GetAttribute(UsdGeomTokens->interpolateBoundary)
            .Get<TfToken>(&interpBoundaryToken);

    // Fall back to the RenderMan for Maya attr.
    if (interpBoundaryToken.IsEmpty()) {
        MPlug plug = mesh.findPlug(MString("rman__torattr___subdivInterp"));
        if (!plug.isNull()) {
            switch (plug.asInt()) {
                case 0:
                    interpBoundaryToken = UsdGeomTokens->none;
                    break;
                case 1:
                    interpBoundaryToken = UsdGeomTokens->edgeAndCorner;
                    break;
                case 2:
                    interpBoundaryToken = UsdGeomTokens->edgeOnly;
                    break;
                default:
                    break;
            }
        }
    }

    if (interpBoundaryToken.IsEmpty()) {
        return TfToken();
    } else if (interpBoundaryToken != UsdGeomTokens->none &&
               interpBoundaryToken != UsdGeomTokens->edgeAndCorner &&
               interpBoundaryToken != UsdGeomTokens->edgeOnly) {
        TF_RUNTIME_ERROR(
                "Unsupported interpolate boundary setting: %s on mesh: %s",
                interpBoundaryToken.GetText(), mesh.fullPathName().asChar());
        return TfToken();
    }

    return interpBoundaryToken;
}

// XXX: Note that this function is not exposed publicly since the USD schema
// has been updated to conform to OpenSubdiv 3. We still look for this attribute
// on Maya nodes specifying this value from OpenSubdiv 2, but we translate the
// value to OpenSubdiv 3. This is to support legacy assets authored against
// OpenSubdiv 2.
static
TfToken
_GetOsd2FVInterpBoundary(const MFnMesh& mesh)
{
    TfToken sdFVInterpBound;

    MPlug plug = mesh.findPlug(MString(
            _meshTokens->USD_faceVaryingInterpolateBoundary.GetText()));
    if (!plug.isNull()) {
        sdFVInterpBound = TfToken(plug.asString().asChar());

        // Translate OSD2 values to OSD3.
        if (sdFVInterpBound == UsdGeomTokens->bilinear) {
            sdFVInterpBound = UsdGeomTokens->all;
        } else if (sdFVInterpBound == UsdGeomTokens->edgeAndCorner) {
            sdFVInterpBound = UsdGeomTokens->cornersPlus1;
        } else if (sdFVInterpBound == _meshTokens->alwaysSharp) {
            sdFVInterpBound = UsdGeomTokens->boundaries;
        } else if (sdFVInterpBound == UsdGeomTokens->edgeOnly) {
            sdFVInterpBound = UsdGeomTokens->none;
        }
    } else {
        plug = mesh.findPlug(MString("rman__torattr___subdivFacevaryingInterp"));
        if (!plug.isNull()) {
            switch(plug.asInt()) {
                case 0:
                    sdFVInterpBound = UsdGeomTokens->all;
                    break;
                case 1:
                    sdFVInterpBound = UsdGeomTokens->cornersPlus1;
                    break;
                case 2:
                    sdFVInterpBound = UsdGeomTokens->none;
                    break;
                case 3:
                    sdFVInterpBound = UsdGeomTokens->boundaries;
                    break;
                default:
                    break;
            }
        }
    }

    return sdFVInterpBound;
}

TfToken UsdMayaMeshUtil::GetSubdivFVLinearInterpolation(const MFnMesh& mesh)
{
    // Try grabbing the value via the adaptor first.
    TfToken sdFVLinearInterpolation;
    UsdMayaAdaptor(mesh.object())
            .GetSchemaOrInheritedSchema<UsdGeomMesh>()
            .GetAttribute(UsdGeomTokens->faceVaryingLinearInterpolation)
            .Get<TfToken>(&sdFVLinearInterpolation);

    // If the OpenSubdiv 3-style face varying linear interpolation value
    // wasn't specified, fall back to the old OpenSubdiv 2-style face
    // varying interpolate boundary value if we have that.
    if (sdFVLinearInterpolation.IsEmpty()) {
        sdFVLinearInterpolation = _GetOsd2FVInterpBoundary(mesh);
    }

    if (!sdFVLinearInterpolation.IsEmpty() &&
            sdFVLinearInterpolation != UsdGeomTokens->all &&
            sdFVLinearInterpolation != UsdGeomTokens->none &&
            sdFVLinearInterpolation != UsdGeomTokens->boundaries &&
            sdFVLinearInterpolation != UsdGeomTokens->cornersOnly &&
            sdFVLinearInterpolation != UsdGeomTokens->cornersPlus1 &&
            sdFVLinearInterpolation != UsdGeomTokens->cornersPlus2) {
        TF_RUNTIME_ERROR(
                "Unsupported face-varying linear interpolation: %s "
                "on mesh: %s",
                sdFVLinearInterpolation.GetText(),
                mesh.fullPathName().asChar());
        return TfToken();
    }

    return sdFVLinearInterpolation;
}

void
UsdMayaMeshUtil::assignPrimvarsToMesh(const UsdGeomMesh& mesh, 
                                      const MObject& meshObj,
                                      const TfToken::Set& excludePrimvarSet)
{
    if(meshObj.apiType() != MFn::kMesh){
        return;
    }

    MFnMesh meshFn(meshObj);

    // GETTING PRIMVARS
    const std::vector<UsdGeomPrimvar> primvars = mesh.GetPrimvars();
    TF_FOR_ALL(iter, primvars) 
    {
        const UsdGeomPrimvar& primvar = *iter;
        const TfToken name = primvar.GetBaseName();
        const TfToken fullName = primvar.GetPrimvarName();
        const SdfValueTypeName typeName = primvar.GetTypeName();
        const TfToken& interpolation = primvar.GetInterpolation();

        // Exclude primvars using the full primvar name without "primvars:".
        // This applies to all primvars; we don't care if it's a color set, a
        // UV set, etc.
        if (excludePrimvarSet.count(fullName) != 0) {
            continue;
        }

        // If the primvar is called either displayColor or displayOpacity check
        // if it was really authored from the user.  It may not have been
        // authored by the user, for example if it was generated by shader
        // values and not an authored colorset/entity.
        // If it was not really authored, we skip the primvar.
        if (name == UsdMayaMeshColorSetTokens->DisplayColorColorSetName ||
              name == UsdMayaMeshColorSetTokens->DisplayOpacityColorSetName) {
          if (!UsdMayaRoundTripUtil::IsAttributeUserAuthored(primvar)) {
              continue;
          }
        }

        // XXX: Maya stores UVs in MFloatArrays and color set data in MColors
        // which store floats, so we currently only import primvars holding
        // float-typed arrays. Should we still consider other precisions
        // (double, half, ...) and/or numeric types (int)?
        if (typeName == SdfValueTypeNames->TexCoord2fArray ||
           (UsdMayaReadUtil::ReadFloat2AsUV() &&
           typeName == SdfValueTypeNames->Float2Array))  {
          // Looks for TexCoord2fArray types for UV sets first
          // Otherwise, if env variable for reading Float2
          // as uv sets is turned on, we assume that Float2Array primvars
          // are UV sets.
          if (!assignUVSetPrimvarToMesh(primvar, meshFn)) {
              TF_WARN("Unable to retrieve and assign data for UV set <%s> on "
                      "mesh <%s>",
                      name.GetText(),
                      mesh.GetPrim().GetPath().GetText());
          }
        } else if (typeName == SdfValueTypeNames->FloatArray ||
                   typeName == SdfValueTypeNames->Float3Array ||
                   typeName == SdfValueTypeNames->Color3fArray ||
                   typeName == SdfValueTypeNames->Float4Array ||
                   typeName == SdfValueTypeNames->Color4fArray) {
          if (!assignColorSetPrimvarToMesh(mesh, primvar, meshFn)) {
              TF_WARN("Unable to retrieve and assign data for color set <%s> "
                      "on mesh <%s>",
                      name.GetText(),
                      mesh.GetPrim().GetPath().GetText());
          }
        } else if (interpolation == UsdGeomTokens->constant) {
          // Constant primvars get added as attributes on the mesh.
          if (!assignConstantPrimvarToMesh(primvar, meshFn)) {
              TF_WARN("Unable to assign constant primvar <%s> as attribute "
                      "on mesh <%s>",
                      name.GetText(),
                      mesh.GetPrim().GetPath().GetText());
          }
        }
    }
}

void 
UsdMayaMeshUtil::assignInvisibleFaces(const UsdGeomMesh& mesh, const MObject& meshObj)
{
    if(meshObj.apiType() != MFn::kMesh){
        return;
    }

    MFnMesh meshFn(meshObj);

    // Set Holes
    VtIntArray holeIndices;
    mesh.GetHoleIndicesAttr().Get(&holeIndices); // not animatable
    if (!holeIndices.empty()) {
        MUintArray mayaHoleIndices;
        mayaHoleIndices.setLength(holeIndices.size());
        for (size_t i = 0u; i < holeIndices.size(); ++i) {
            mayaHoleIndices[i] = holeIndices[i];
        }

        if (meshFn.setInvisibleFaces(mayaHoleIndices) != MS::kSuccess) {
            TF_RUNTIME_ERROR("Unable to set Invisible Faces on <%s>",
                             meshFn.fullPathName().asChar());
        }
    }
}

MStatus
UsdMayaMeshUtil::assignSubDivTagsToMesh( const UsdGeomMesh& mesh,
                                         MObject& meshObj,
                                         MFnMesh& meshFn )
{
    // We may want to provide the option in the future, but for now, we
    // default to using crease sets when setting crease data.
    //
    const bool USE_CREASE_SETS = true;

    MStatus statusOK{MS::kSuccess};

    MDagPath meshPath;
    statusOK = MDagPath::getAPathTo(meshObj,meshPath);

    if (!statusOK){
        return MS::kFailure;
    }

    // USD does not support grouped verts and edges, so combine all components
    // with the same weight into one set to reduce the overall crease set
    // count. The user can always split the sets up later if desired.
    // 
    // This structure is unused if crease sets aren't being created.
    std::unordered_map<float,MSelectionList> elemsPerWeight;

    // Vert Creasing
    VtArray<int>   subdCornerIndices;
    VtArray<float> subdCornerSharpnesses;
    mesh.GetCornerIndicesAttr().Get(&subdCornerIndices); // not animatable
    mesh.GetCornerSharpnessesAttr().Get(&subdCornerSharpnesses); // not animatable
    if (!subdCornerIndices.empty()) {
        if (subdCornerIndices.size() == subdCornerSharpnesses.size() ) {
            statusOK.clear();

            if (USE_CREASE_SETS) {
                MItMeshVertex vertIt(meshObj);
                for (unsigned int i=0; i < subdCornerIndices.size(); i++) {

                    // Ignore zero-sharpness corners
                    if (subdCornerSharpnesses[i]==0)
                        continue;

                    MSelectionList &elemList =
                        elemsPerWeight[ subdCornerSharpnesses[i] ];

                    int prevIndexDummy; // dummy param
                    statusOK = vertIt.setIndex(subdCornerIndices[i], prevIndexDummy);
                    if (!statusOK)
                        break;
                    statusOK = elemList.add(meshPath,vertIt.currentItem());
                    if (!statusOK)
                        break;
                }

            } else {
                MUintArray   mayaCreaseVertIds;
                MDoubleArray mayaCreaseVertValues;
                mayaCreaseVertIds.setLength( subdCornerIndices.size() );
                mayaCreaseVertValues.setLength( subdCornerIndices.size() );
                for (unsigned int i=0; i < subdCornerIndices.size(); i++) {

                    // Ignore zero-sharpness corners
                    if (subdCornerSharpnesses[i]==0)
                        continue;

                    mayaCreaseVertIds[i] = subdCornerIndices[i];
                    mayaCreaseVertValues[i] = subdCornerSharpnesses[i];
                }
                statusOK = meshFn.setCreaseVertices(mayaCreaseVertIds, mayaCreaseVertValues);
            }

            if (!statusOK) {
                TF_RUNTIME_ERROR("Unable to set Crease Vertices on <%s>: %s", 
                        meshFn.fullPathName().asChar(),
                        statusOK.errorString().asChar());
                return MS::kFailure;
            }

        } else {
            TF_RUNTIME_ERROR(
                    "Mismatch between Corner Indices & Sharpness on <%s>", 
                    mesh.GetPrim().GetPath().GetText());
            return MS::kFailure;
        }
    }

    // Edge Creasing
    VtArray<int>   subdCreaseLengths;
    VtArray<int>   subdCreaseIndices;
    VtArray<float> subdCreaseSharpnesses;
    mesh.GetCreaseLengthsAttr().Get(&subdCreaseLengths);
    mesh.GetCreaseIndicesAttr().Get(&subdCreaseIndices);
    mesh.GetCreaseSharpnessesAttr().Get(&subdCreaseSharpnesses);
    if (!subdCreaseLengths.empty()) {
        if (subdCreaseLengths.size() == subdCreaseSharpnesses.size() ) {
            MUintArray   mayaCreaseEdgeIds;
            MDoubleArray mayaCreaseEdgeValues;
            MIntArray connectedEdges;
            unsigned int creaseIndexBase = 0;

            statusOK.clear();

            for (unsigned int creaseGroup=0;
                    statusOK && creaseGroup<subdCreaseLengths.size();
                    creaseIndexBase+=subdCreaseLengths[creaseGroup++]) {

                // Ignore zero-sharpness creases
                if (subdCreaseSharpnesses[creaseGroup]==0)
                    continue;

                MItMeshVertex vertIt(meshObj);
                MItMeshEdge edgeIt(meshObj);

                for (int i=0; statusOK && i<subdCreaseLengths[creaseGroup]-1; i++) {
                    // Find the edgeId associated with the 2 vertIds.
                    int prevIndexDummy; // dummy param
                    statusOK = vertIt.setIndex(subdCreaseIndices[creaseIndexBase+i], prevIndexDummy);
                    if (!statusOK)
                        break;
                    statusOK = vertIt.getConnectedEdges(connectedEdges);
                    if (!statusOK)
                        break;

                    int edgeIndex = -1;
                    for (unsigned int e=0; statusOK && e<connectedEdges.length(); e++) {
                        int tmpOppositeVertexId;
                        statusOK = vertIt.getOppositeVertex(tmpOppositeVertexId, connectedEdges[e]);
                        if (!statusOK)
                            break;
                        if ( subdCreaseIndices[creaseIndexBase+i+1] == tmpOppositeVertexId ) {
                            edgeIndex = connectedEdges[e];
                            break;
                        }
                    }
                    if (statusOK && edgeIndex != -1) {
                        if (USE_CREASE_SETS) {
                            int prevIndexDummy; // dummy param
                            statusOK = edgeIt.setIndex(edgeIndex,prevIndexDummy);
                            if (!statusOK)
                                break;
                            statusOK = elemsPerWeight[subdCreaseSharpnesses[creaseGroup]].
                                            add(meshPath,edgeIt.currentItem());
                            if (!statusOK)
                                break;
                        } else {
                            mayaCreaseEdgeIds.append(edgeIndex);
                            mayaCreaseEdgeValues.append(subdCreaseSharpnesses[creaseGroup]);
                        }
                    }
                }
            }

            if (statusOK && !USE_CREASE_SETS) {
                statusOK = meshFn.setCreaseEdges(mayaCreaseEdgeIds, mayaCreaseEdgeValues);
            }

            if (!statusOK) {
                TF_RUNTIME_ERROR("Unable to set Crease Edges on <%s>: %s", 
                        meshFn.fullPathName().asChar(),
                        statusOK.errorString().asChar());
                return MS::kFailure;
            }

        } else {
            TF_RUNTIME_ERROR(
                    "Mismatch between Crease Lengths & Sharpness on <%s>", 
                    mesh.GetPrim().GetPath().GetText());
            return MS::kFailure;
        }
    }

    if (USE_CREASE_SETS) {
        TF_FOR_ALL(weightList, elemsPerWeight) {
            double creaseLevel = weightList->first;
            MSelectionList &elemList = weightList->second;

            if (!addCreaseSet( meshFn.name().asChar(),
                                creaseLevel, elemList, &statusOK )){
                TF_RUNTIME_ERROR("Unable to set crease sets on <%s>: %s", 
                        meshFn.fullPathName().asChar(),
                        statusOK.errorString().asChar());
                return MS::kFailure;
            }
        }
    }

    return MS::kSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
