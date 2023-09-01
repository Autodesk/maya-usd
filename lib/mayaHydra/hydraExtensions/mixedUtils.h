//
// Copyright 2019 Luma Pictures
// Copyright 2023 Autodesk, Inc. All rights reserved.
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

#ifndef MAYAHYDRALIB_MIXED_UTILS_H
#define MAYAHYDRALIB_MIXED_UTILS_H

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/mayaHydra.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFloatMatrix.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MMatrix.h>
#include <maya/MObject.h>

namespace MAYAHYDRA_NS_DEF {

/**
 * @brief Converts a Maya matrix to a double precision GfMatrix.
 *
 * @param[in] mayaMat Maya `MMatrix` to be converted.
 *
 * @return `GfMatrix4d` equal to \p mayaMat.
 */
inline pxr::GfMatrix4d GetGfMatrixFromMaya(const MMatrix& mayaMat)
{
    pxr::GfMatrix4d mat;
    memcpy(mat.GetArray(), mayaMat[0], sizeof(double) * 16);
    return mat;
}

/**
 * @brief Converts a Maya float matrix to a double precision GfMatrix.
 *
 * @param[in] mayaMat Maya `MFloatMatrix` to be converted.
 *
 * @return `GfMatrix4d` equal to \p mayaMat.
 */
inline pxr::GfMatrix4d GetGfMatrixFromMaya(const MFloatMatrix& mayaMat)
{
    pxr::GfMatrix4d mat;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j)
            mat[i][j] = mayaMat(i, j);
    }
    return mat;
}

/**
 * @brief Returns the texture file path from a "file" shader node.
 *
 * @param[in] fileNode "file" shader node.
 *
 * @return Full path to the texture pointed used by the file node. `<UDIM>` tags are kept intact.
 */
MAYAHYDRALIB_API
pxr::TfToken GetFileTexturePath(const MFnDependencyNode& fileNode);

/**
 * @brief Determines whether or not a given DagPath refers to a shape.
 *
 * @param[in] dagPath is the DagPath to the potential shape.
 *
 * @return True if the dagPath refers to a shape, false otherwise.
 */
MAYAHYDRALIB_API
bool IsShape(const MDagPath& dagPath);

/**
 * @brief Converts the given Maya MDagPath \p dagPath into an SdfPath.
 *
 * @usage Converts the given Maya MDagPath \p dagPath into an SdfPath. Elements of the path will be
 * sanitized such that it is a valid SdfPath. If \p mergeTransformAndShape is true and \p dagPath is
 * a shape node, it will return the parent SdfPath of the shape's SdfPath, such that the transform
 * and the shape have the same SdfPath.
 *
 * @param[in] dagPath is the DAG path to convert to an SdfPath.
 * @param[in] mergeTransformAndShape determines whether or not to consider the transform and shape
 * paths as one.
 * @param[in] stripNamespaces determines whether or not to strip namespaces from the path.
 *
 * @return The SdfPath corresponding to the given DAG path.
 */
MAYAHYDRALIB_API
pxr::SdfPath DagPathToSdfPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces);

/**
 * @brief Creates an SdfPath from the given Maya MRenderItem.
 *
 * @usage Creates an SdfPath from the given Maya MRenderItem. Elements of the path will be sanitized
 * such that it is a valid SdfPath.
 *
 * @param[in] ri is the MRenderItem to create the SdfPath for.
 * @param[in] stripNamespaces determines whether or not to strip namespaces from the path.
 *
 * @return The SdfPath corresponding to the given MRenderItem.
 */
MAYAHYDRALIB_API
pxr::SdfPath RenderItemToSdfPath(const MRenderItem& ri, const bool stripNamespaces);

} // namespace MAYAHYDRA_NS_DEF

#endif // MAYAHYDRALIB_MIXED_UTILS_H
