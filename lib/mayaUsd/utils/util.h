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
#ifndef PXRUSDMAYA_UTIL_H
#define PXRUSDMAYA_UTIL_H

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/tf/declarePtrs.h>
#include <pxr/base/tf/refPtr.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/dictionary.h>
#include <pxr/base/vt/types.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/timeCode.h>

#include <maya/MArgDatabase.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MDistance.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericData.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// General utilities for working with the Maya API.
namespace UsdMayaUtil {

struct _CmpDag
{
    bool operator()(const MDagPath& lhs, const MDagPath& rhs) const
    {
        int pathCountDiff = lhs.pathCount() - rhs.pathCount();
        return (0 != pathCountDiff)
            ? (pathCountDiff < 0)
            : (strcmp(lhs.fullPathName().asChar(), rhs.fullPathName().asChar()) < 0);
    }
};

/// Set of DAG paths.
/// Warning: MDagPaths refer to specific objects, so the internal fullPathName
/// may change over time. Only use this class if you can guarantee that DAG
/// nodes won't be renamed or reparented while class instances are alive.
/// Otherwise, you may see inconsistent results.
using MDagPathSet = std::set<MDagPath, _CmpDag>;

/// Mapping of DAG paths to an arbitrary type.
/// Warning: MDagPaths refer to specific objects, so the internal fullPathName
/// may change over time. Only use this class if you can guarantee that DAG
/// nodes won't be renamed or reparented while class instances are alive.
/// Otherwise, you may see inconsistent results.
template <typename V> using MDagPathMap = std::map<MDagPath, V, _CmpDag>;

struct _HashObjectHandle
{
    unsigned long operator()(const MObjectHandle& handle) const { return handle.hashCode(); }
};

/// Unordered set of Maya object handles.
using MObjectHandleUnorderedSet = std::unordered_set<MObjectHandle, _HashObjectHandle>;

/// Unordered mapping of Maya object handles to an arbitrary type.
template <typename V>
using MObjectHandleUnorderedMap = std::unordered_map<MObjectHandle, V, _HashObjectHandle>;

/// RAII-style helper for destructing an MDataHandle obtained from a plug
/// once it goes out of scope.
class MDataHandleHolder : public TfRefBase
{
    MPlug       _plug;
    MDataHandle _dataHandle;

public:
    MAYAUSD_CORE_PUBLIC
    static TfRefPtr<MDataHandleHolder> New(const MPlug& plug);
    MAYAUSD_CORE_PUBLIC
    MDataHandle GetDataHandle() { return _dataHandle; }

private:
    MDataHandleHolder(const MPlug& plug, MDataHandle dataHandle);
    ~MDataHandleHolder() override;
};

const double MillimetersPerInch = 25.4;

/// Converts the given value \p mm in millimeters to the equivalent value
/// in inches.
inline double ConvertMMToInches(const double mm) { return mm / MillimetersPerInch; }

/// Converts the given value \p inches in inches to the equivalent value
/// in millimeters.
inline double ConvertInchesToMM(const double inches) { return inches * MillimetersPerInch; }

/// Converts the given value \p d from units \p from to the equivalent value in units \p
inline double ConvertUnit(double d, double from, double to)
{
    return from == to ? d : d * from / to;
}

const double MillimetersPerCentimeter = 10.0;

/// Converts the given value \p mm in millimeters to the equivalent value
/// in centimeters.
inline double ConvertMMToCM(const double mm) { return mm / MillimetersPerCentimeter; }

/// Converts the given value \p cm in centimeters to the equivalent value
/// in millimeters.
inline double ConvertCMToMM(const double cm) { return cm * MillimetersPerCentimeter; }

/// Converts the given value \p mdistance in Maya's MDistance units to the
/// equivalent value in USD's metersPerUnit.
MAYAUSD_CORE_PUBLIC
double ConvertMDistanceUnitToUsdGeomLinearUnit(const MDistance::Unit mdistanceUnit);

/// Coverts the given value \p linearUnit in USD's metersPerUnit to the
/// equivalent value in Maya's MDistance units.
MAYAUSD_CORE_PUBLIC
MDistance::Unit ConvertUsdGeomLinearUnitToMDistanceUnit(const double linearUnit);

/// Get the full name of the Maya node \p mayaNode.
///
/// If \p mayaNode refers to a DAG node (i.e. supports the MFnDagNode function
/// set), then the name returned will be the DAG node's full path name.
///
/// If \p mayaNode refers to a DG node (i.e. supports the MFnDependencyNode
/// function set), then the name returned will be the DG node's absolute name.
///
/// If \p mayaNode is not one of these or if an error is encountered, an
/// empty string will be returned.
MAYAUSD_CORE_PUBLIC
std::string GetMayaNodeName(const MObject& mayaNode);

/**
 * Gets the minimum unique name of a DAG node.
 *
 * @param node  The node to find the name of.
 *
 * @return      The name as a Maya string.
 */
MAYAUSD_CORE_PUBLIC
MString GetUniqueNameOfDagNode(const MObject& node);

/// Gets the Maya MObject for the node named \p nodeName.
MAYAUSD_CORE_PUBLIC
MStatus GetMObjectByName(const std::string& nodeName, MObject& mObj);

/// Gets the Maya MObject for the node named \p nodeName.
MAYAUSD_CORE_PUBLIC
MStatus GetMObjectByName(const MString& nodeName, MObject& mObj);

/// Gets the UsdStage for the proxy shape  node named \p nodeName.
MAYAUSD_CORE_PUBLIC
UsdStageRefPtr GetStageByProxyName(const std::string& nodeName);

/// Gets the Maya MPlug for the given \p attrPath.
/// The attribute path should be specified as "nodeName.attrName" (the format
/// used by MEL).
MAYAUSD_CORE_PUBLIC
MStatus GetPlugByName(const std::string& attrPath, MPlug& plug);

/**
 * Finds a child plug with the given `name`.
 *
 * @param parent    The parent plug to start the search from.
 * @param name      The name of the child plug to find. This should be the short name.
 *
 * @return          The plug if it can be found, or a null `MPlug` otherwise.
 */
MAYAUSD_CORE_PUBLIC
MPlug FindChildPlugWithName(const MPlug& parent, const MString& name);

/// Get the MPlug for the output time attribute of Maya's global time object
///
/// The Maya API does not appear to provide any facilities for getting a handle
/// to the global time object (e.g. "time1"). We need to find this object in
/// order to make connections between its "outTime" attribute and the input
/// "time" attributes on assembly nodes when their "Playback" representation is
/// activated.
///
/// This function makes a best effort attempt to find "time1" by looking through
/// all MFn::kTime function set objects in the scene and returning the one whose
/// outTime attribute matches the current time. If no such object can be found,
/// an invalid plug is returned.
MAYAUSD_CORE_PUBLIC
MPlug GetMayaTimePlug();

/// Get the MPlug for the shaders attribute of Maya's defaultShaderList
///
/// This is an accessor for the "defaultShaderList1.shaders" plug.  Similar to
/// GetMayaTimePlug(), it will traverse through MFn::kShaderList objects.
MAYAUSD_CORE_PUBLIC
MPlug GetMayaShaderListPlug();

/// Get the MObject for the DefaultLightSet, which should add any light nodes
/// as members for them to take effect in the scene
MAYAUSD_CORE_PUBLIC
MObject GetDefaultLightSetObject();

MAYAUSD_CORE_PUBLIC
bool isAncestorDescendentRelationship(const MDagPath& path1, const MDagPath& path2);

// returns 0 if static, 1 if sampled, and 2 if a curve
MAYAUSD_CORE_PUBLIC
int getSampledType(const MPlug& iPlug, const bool includeConnectedChildren);

/// Determine if the Maya object \p mayaObject is animated or not
MAYAUSD_CORE_PUBLIC
bool isAnimated(const MObject& mayaObject, const bool checkParent = false);

// Determine if a specific Maya plug is animated or not.
MAYAUSD_CORE_PUBLIC
bool isPlugAnimated(const MPlug& plug);

/// Determine if a Maya object is an intermediate object.
///
/// Only objects with the MFnDagNode function set can be intermediate objects.
/// Objects whose intermediate object status cannot be determined are assumed
/// not to be intermediate objects.
MAYAUSD_CORE_PUBLIC
bool isIntermediate(const MObject& object);

// returns true for visible and lod invisible and not templated objects
MAYAUSD_CORE_PUBLIC
bool isRenderable(const MObject& object);

/// Determine whether a Maya object can be saved to or exported from the Maya
/// scene.
///
/// Objects whose "default node" or "do not write" status cannot be determined
/// using the MFnDependencyNode function set are assumed to be writable.
MAYAUSD_CORE_PUBLIC
bool isWritable(const MObject& object);

/// This is the delimiter that Maya uses to identify levels of hierarchy in the
/// Maya DAG.
const std::string MayaDagDelimiter("|");

/// This is the delimiter that Maya uses to separate levels of namespace in
/// Maya node names.
const std::string MayaNamespaceDelimiter(":");

/// Strip \p nsDepth namespaces from \p nodeName.
///
/// This will turn "taco:foo:bar" into "foo:bar" for \p nsDepth == 1, or
/// "taco:foo:bar" into "bar" for \p nsDepth > 1.
/// If \p nsDepth is -1, all namespaces are stripped.
MAYAUSD_CORE_PUBLIC
std::string stripNamespaces(const std::string& nodeName, const int nsDepth = -1);

MAYAUSD_CORE_PUBLIC
std::string SanitizeName(const std::string& name);

// This to allow various pipeline to sanitize the colorset name for output
MAYAUSD_CORE_PUBLIC
std::string SanitizeColorSetName(const std::string& name);

/// Get the base colors and opacities from the shader(s) bound to \p node.
/// Returned colors will be in linear color space.
///
/// A single value for each of color and alpha will be returned,
/// interpolation will be constant, and assignmentIndices will be empty.
///
MAYAUSD_CORE_PUBLIC
bool GetLinearShaderColor(
    const MFnDagNode&     node,
    PXR_NS::VtVec3fArray* RGBData,
    PXR_NS::VtFloatArray* AlphaData,
    PXR_NS::TfToken*      interpolation,
    PXR_NS::VtIntArray*   assignmentIndices);

/// Get the base colors and opacities from the shader(s) bound to \p mesh.
/// Returned colors will be in linear color space.
///
/// If the entire mesh has a single shader assignment, a single value for each
/// of color and alpha will be returned, interpolation will be constant, and
/// assignmentIndices will be empty.
///
/// Otherwise, a color and alpha value will be returned for each shader
/// assigned to any face of the mesh. \p assignmentIndices will be the length
/// of the number of faces with values indexing into the color and alpha arrays
/// representing per-face assignments. Faces with no assigned shader will have
/// a value of -1 in \p assignmentIndices. \p interpolation will be uniform.
///
MAYAUSD_CORE_PUBLIC
bool GetLinearShaderColor(
    const MFnMesh&        mesh,
    PXR_NS::VtVec3fArray* RGBData,
    PXR_NS::VtFloatArray* AlphaData,
    PXR_NS::TfToken*      interpolation,
    PXR_NS::VtIntArray*   assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
MAYAUSD_CORE_PUBLIC
void MergeEquivalentIndexedValues(
    PXR_NS::VtFloatArray* valueData,
    PXR_NS::VtIntArray*   assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
MAYAUSD_CORE_PUBLIC
void MergeEquivalentIndexedValues(
    PXR_NS::VtVec2fArray* valueData,
    PXR_NS::VtIntArray*   assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
MAYAUSD_CORE_PUBLIC
void MergeEquivalentIndexedValues(
    PXR_NS::VtVec3fArray* valueData,
    PXR_NS::VtIntArray*   assignmentIndices);

/// Combine distinct indices that point to the same values to all point to the
/// same index for that value. This will potentially shrink the data array.
MAYAUSD_CORE_PUBLIC
void MergeEquivalentIndexedValues(
    PXR_NS::VtVec4fArray* valueData,
    PXR_NS::VtIntArray*   assignmentIndices);

/// Attempt to compress faceVarying primvar indices to uniform, vertex, or
/// constant interpolation if possible. This will potentially shrink the
/// indices array and will update the interpolation if any compression was
/// possible.
MAYAUSD_CORE_PUBLIC
void CompressFaceVaryingPrimvarIndices(
    const MFnMesh&      mesh,
    PXR_NS::TfToken*    interpolation,
    PXR_NS::VtIntArray* assignmentIndices);

/// Get whether \p plug is authored in the Maya scene.
///
/// A plug is considered authored if its value has been changed from the
/// default (or since being brought in from a reference for plugs on nodes from
/// referenced files), or if the plug is the destination of a connection.
/// Otherwise, it is considered unauthored.
MAYAUSD_CORE_PUBLIC
bool IsAuthored(const MPlug& plug);

MAYAUSD_CORE_PUBLIC
bool IsPlugDefaultValue(const MPlug& plug);

MAYAUSD_CORE_PUBLIC
MPlug GetConnected(const MPlug& plug);

MAYAUSD_CORE_PUBLIC
void Connect(const MPlug& srcPlug, const MPlug& dstPlug, const bool clearDstPlug);

MAYAUSD_CORE_PUBLIC
void Connect(
    const MPlug& srcPlug,
    const MPlug& dstPlug,
    const bool   clearDstPlug,
    MDGModifier& dgMod);

/// Get a named child plug of \p plug by name.
MAYAUSD_CORE_PUBLIC
MPlug FindChildPlugByName(const MPlug& plug, const MString& name);

/// Converts the given Maya node name \p nodeName into an SdfPath.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath MayaNodeNameToSdfPath(const std::string& nodeName, const bool stripNamespaces);

/// Converts the given Maya MDagPath \p dagPath into an SdfPath.
///
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath MDagPathToUsdPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces);

/// Converts the given Maya MDagPath \p dagPath into an SdfPath.
///
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath RenderItemToUsdPath(
	const MRenderItem& ri,
	const bool      mergeTransformAndShape,
	const bool      stripNamespaces);

/// Converts the given Maya MDagPath \p dagPath into an SdfPath.
///
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath RenderItemShaderToUsdPath(
	const MRenderItem& ri,
	const MShaderInstance& shader,
	const bool      mergeTransformAndShape,
	const bool      stripNamespaces);

/// Convenience function to retrieve custom data
MAYAUSD_CORE_PUBLIC
bool GetBoolCustomData(
    const PXR_NS::UsdAttribute& obj,
    const PXR_NS::TfToken&      key,
    const bool                  defaultValue);

/// Compute the value of \p attr, returning true upon success.
template <typename T>
bool getPlugValue(
    const MFnDependencyNode& depNode,
    const MString&           attr,
    T*                       val,
    bool*                    isAnimated = nullptr)
{
    MPlug plg = depNode.findPlug(attr, /* wantNetworkedPlug = */ true);
    if (plg.isNull()) {
        return false;
    }

    if (isAnimated) {
        *isAnimated = isPlugAnimated(plg);
    }

    return plg.getValue(*val);
}

/// Convert a Gf matrix to an MMatrix.
MAYAUSD_CORE_PUBLIC
MMatrix GfMatrixToMMatrix(const GfMatrix4d& mx);

// Like getPlugValue, but gets the matrix stored inside the MFnMatrixData on a
// plug.
// Returns true upon success, placing the matrix in the outVal parameter.
MAYAUSD_CORE_PUBLIC
bool getPlugMatrix(const MFnDependencyNode& depNode, const MString& attr, MMatrix* outVal);

/// Set a matrix value on plug name \p attr, of \p depNode.
/// Returns true if the value was set on the plug successfully, false otherwise.
MAYAUSD_CORE_PUBLIC
bool setPlugMatrix(const MFnDependencyNode& depNode, const MString& attr, const GfMatrix4d& mx);

MAYAUSD_CORE_PUBLIC
bool setPlugMatrix(const GfMatrix4d& mx, MPlug& plug);

/// Given an \p usdAttr , extract the value at the default timecode and write
/// it on \p attrPlug.
/// This will make sure that color values (which are linear in usd) get
/// gamma corrected (display in maya).
/// Returns true if the value was set on the plug successfully, false otherwise.
MAYAUSD_CORE_PUBLIC
bool setPlugValue(const PXR_NS::UsdAttribute& attr, MPlug& attrPlug);

/// Given an \p usdAttr , extract the value at timecode \p time and write it
/// on \p attrPlug.
/// This will make sure that color values (which are linear in usd) get
/// gamma corrected (display in maya).
/// Returns true if the value was set on the plug successfully, false otherwise.
MAYAUSD_CORE_PUBLIC
bool setPlugValue(
    const PXR_NS::UsdAttribute& attr,
    const PXR_NS::UsdTimeCode   time,
    MPlug&                      attrPlug);

/// \brief sets \p attr to have value \p val, assuming it exists on \p
/// depNode.  Returns true if successful.
template <typename T>
bool setPlugValue(const MFnDependencyNode& depNode, const MString& attr, const T& val)
{
    MPlug plg = depNode.findPlug(attr, /* findNetworked = */ false);
    if (plg.isNull()) {
        return false;
    }

    return plg.setValue(val);
}

/// Obtains an RAII helper object for accessing the MDataHandle stored on the
/// plug. When the helper object goes out of scope, the data handle will be
/// destructed.
/// If the plug's data handle could not be obtained, returns nullptr.
MAYAUSD_CORE_PUBLIC
TfRefPtr<MDataHandleHolder> GetPlugDataHandle(const MPlug& plug);

MAYAUSD_CORE_PUBLIC
bool SetNotes(MFnDependencyNode& depNode, const std::string& notes);

MAYAUSD_CORE_PUBLIC
bool SetHiddenInOutliner(MFnDependencyNode& depNode, const bool hidden);

/// Reads values from the given \p argData into a VtDictionary, using the
/// \p guideDict to figure out which keys and what type of values should be read
/// from \p argData.
/// Mainly useful for parsing arguments in commands all at once.
MAYAUSD_CORE_PUBLIC
VtDictionary
GetDictionaryFromArgDatabase(const MArgDatabase& argData, const VtDictionary& guideDict);

/// Parses \p value based on the type of \p key in \p guideDict, returning the
/// parsed value wrapped in a VtValue.
/// Raises a coding error if \p key doesn't exist in \p guideDict.
/// Mainly useful for parsing arguments one-by-one in translators' option
/// strings. If you have an MArgList/MArgParser/MArgDatabase, it's going to be
/// way simpler to use GetDictionaryFromArgDatabase() instead.
MAYAUSD_CORE_PUBLIC
VtValue
ParseArgumentValue(const std::string& key, const std::string& value, const VtDictionary& guideDict);

/// Converts a value into a string that can be parsed back using ParseArgumentValue.
/// Should be used when generating argument strings that are to be used by translators.
/// Has the same rules and limitations as the former function.
/// Will return false if the variant type is not supported.
MAYAUSD_CORE_PUBLIC
std::pair<bool, std::string> ValueToArgument(const VtValue& value);

/// Gets all Maya node types that are ancestors of the given Maya node type
/// \p ty. If \p ty isn't registered in Maya's type system, issues a runtime
/// error and returns an empty string.
/// The returned list is sorted from furthest to closest ancestor. The returned
/// list will always have the given type \p ty as the last item.
/// Note that this calls out to MEL.
MAYAUSD_CORE_PUBLIC
std::vector<std::string> GetAllAncestorMayaNodeTypes(const std::string& ty);

/// If dagPath is a scene assembly node or is the descendant of one, populates
/// the \p *assemblyPath with the assembly path and returns \c true.
/// Otherwise, returns \c false.
MAYAUSD_CORE_PUBLIC
bool FindAncestorSceneAssembly(const MDagPath& dagPath, MDagPath* assemblyPath = nullptr);

MAYAUSD_CORE_PUBLIC
MBoundingBox GetInfiniteBoundingBox();

MAYAUSD_CORE_PUBLIC
MString convert(const std::string&);

MAYAUSD_CORE_PUBLIC
MString convert(const TfToken& token);

MAYAUSD_CORE_PUBLIC
std::string convert(const MString&);

/// Retrieve all descendant nodes, including self.
MAYAUSD_CORE_PUBLIC
std::vector<MDagPath> getDescendants(const MDagPath& path);

/// Retrieve all descendant nodes, including self, but starting from the most
/// distant grand-children.
MAYAUSD_CORE_PUBLIC
std::vector<MDagPath> getDescendantsStartingWithChildren(const MDagPath& path);

MAYAUSD_CORE_PUBLIC
MDagPath getDagPath(const MFnDependencyNode& depNodeFn, const bool reportError = true);

MAYAUSD_CORE_PUBLIC
MDagPathMap<SdfPath> getDagPathMap(const MFnDependencyNode& depNodeFn, const SdfPath& usdPath);

MAYAUSD_CORE_PUBLIC
VtIntArray shiftIndices(const VtIntArray& array, int shift);

template <typename T> VtValue PushFirstValue(VtArray<T> arr, const T& value)
{
    arr.resize(arr.size() + 1);
    std::move_backward(arr.begin(), arr.end() - 1, arr.end());
    arr[0] = value;
    return VtValue(arr);
}

MAYAUSD_CORE_PUBLIC
VtValue pushFirstValue(const VtValue& arr, const VtValue& defaultValue);

template <typename T> VtValue PopFirstValue(VtArray<T> arr)
{
    std::move(arr.begin() + 1, arr.end(), arr.begin());
    arr.pop_back();
    return VtValue(arr);
}

MAYAUSD_CORE_PUBLIC
VtValue popFirstValue(const VtValue& arr);

MAYAUSD_CORE_PUBLIC
bool containsUnauthoredValues(const VtIntArray& indices);

MAYAUSD_CORE_PUBLIC
MDagPath nameToDagPath(const std::string& name);

/// Utility function used by the export translator and commands to filter out objects to export
/// by hierarchy.  When an object is exported then all of its children are exported as well, so
/// the children should be removed from the list before the set of objects ends up in the write
/// job, where it would error out.
/// If \p exportSelected is true then the active selection list will be added to \p objectList
/// and then used to fill \p dagPaths with the objects to be exported.
/// If \p exportSelected is false and \p objectList is not empty then \p objectList will
/// be used to fill \p dagPaths with the objects to be exported.
/// If \p exportSelected is false and \p objectList is empty then all objects starting at
/// the DAG root will be added to \p objectList and then used to fill \p dagPaths with
/// the objects to be exported.
///
MAYAUSD_CORE_PUBLIC
void GetFilteredSelectionToExport(
    bool                      exportSelected,
    MSelectionList&           objectList,
    UsdMayaUtil::MDagPathSet& dagPaths);

/// Coverts a given \pMTime::\pUnit enum to a \pdouble value of samples per second
/// Returns 0.0 if the result is invalid.
MAYAUSD_CORE_PUBLIC
double ConvertMTimeUnitToDouble(const MTime::Unit& unit);

/// Get's the scene's \pMTime::\pUnit as a \pdouble value of samples per second
/// Returns 0.0 if the result is invalid.
MAYAUSD_CORE_PUBLIC
double GetSceneMTimeUnitAsDouble();

/**
 * Searches the given array for an element.
 *
 * @param a         The element to search for.
 * @param array     The array to search within.
 * @param idx       Storage for the index of the element within the array if it exists.
 *                  If it does not exist, this will be undefined. May be set to `NULL` if
 *                  you're only interested in finding out whether the element exists, and
 *                  not where the element is actually located in the array.
 *
 * @return          Returns ``true`` if the element exists in the array, ``false`` otherwise.
 */
MAYAUSD_CORE_PUBLIC
bool mayaSearchMIntArray(const int a, const MIntArray& array, unsigned int* idx = nullptr);

MAYAUSD_CORE_PUBLIC
MStatus GetAllIndicesFromComponentListDataPlug(const MPlug& plg, MIntArray& indices);

/**
 * Checks if the given mesh has any blendshape deformers driving it.
 *
 * @param mesh      The mesh to check.
 *
 * @return          ``true`` if the mesh is being driven by blendshape deformers, ``false``
 * otherwise.
 */
MAYAUSD_CORE_PUBLIC
bool CheckMeshUpstreamForBlendShapes(const MObject& mesh);

/**
 * Returns the current Maya project path, also known as the workspace.
 *
 * @return     The current Maya project path. If no project is set or an error occurred, returns an
 * empty string.
 */
MAYAUSD_CORE_PUBLIC
MString GetCurrentMayaWorkspacePath();

MAYAUSD_CORE_PUBLIC
MString GetCurrentSceneFilePath();

/**
 * Returns all the sublayers recursively for a given layer
 *
 * @param layer   The layer to start from and get the sublayers
 *
 * @return The list of identifiers for all the sublayers
 */
MAYAUSD_CORE_PUBLIC
std::set<std::string> getAllSublayers(const PXR_NS::SdfLayerRefPtr& layer);

/**
 * Returns all the sublayers recursively for a list of layers
 *
 * @param parentLayerPaths   The list of the parent layer paths to traverse recursively
 * @param includeParents   This will add the parents passed in to the output
 *
 * @return The list of identifiers for all the sublayers for each parent (plus the parents
 * potentially)
 */
MAYAUSD_CORE_PUBLIC
std::set<std::string>
getAllSublayers(const std::vector<std::string>& parentLayerPaths, bool includeParents = false);

/// Takes the supplied bounding box and adds to it Maya-specific extents
/// that come from the nodes originating from the supplied root node
MAYAUSD_CORE_PUBLIC
void AddMayaExtents(
    PXR_NS::GfBBox3d&         bbox,
    const PXR_NS::UsdPrim&    root,
    const PXR_NS::UsdTimeCode time);

} // namespace UsdMayaUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif
