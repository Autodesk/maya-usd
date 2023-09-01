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

#ifndef MAYAHYDRALIB_HYDRA_UTILS_H
#define MAYAHYDRALIB_HYDRA_UTILS_H

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/mayaHydra.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/sceneIndex.h>
#include <pxr/usd/sdf/path.h>

#include <string>

namespace MAYAHYDRA_NS_DEF {

/**
 * @brief Return the \p VtValue type and value as a string for debugging purposes.
 *
 * @param[in] val is the \p VtValue to be converted.
 * 
 * @return The \p VtValue type and value in string form.
 */
MAYAHYDRALIB_API
std::string ConvertVtValueToString(const pxr::VtValue& val);

/**
 * @brief Strip \p nsDepth namespaces from \p nodeName.
 *
 * @usage This will turn "taco:foo:bar" into "foo:bar" for \p nsDepth == 1, or "taco:foo:bar" into
 * "bar" for \p nsDepth > 1. If \p nsDepth is -1, all namespaces are stripped.
 *
 * @param[in] nodeName is the node name from which to strip namespaces.
 * @param[in] nsDepth is the namespace depth to strip.
 *
 * @return The stripped version of \p nodeName.
 */
MAYAHYDRALIB_API
std::string StripNamespaces(const std::string& nodeName, const int nsDepth = -1);

/**
 * @brief Replaces the invalid characters for SdfPath in-place in \p inOutPathString.
 *
 * @param[in,out] inOutPathString is the path string to sanitize.
 * @param[in] doStripNamespaces determines whether to strip namespaces or not.
 */
MAYAHYDRALIB_API
void SanitizeNameForSdfPath(std::string& inOutPathString, bool doStripNamespaces = false);

/**
 * @brief Get the given SdfPath without its parent path.
 *
 * @usage Get the given SdfPath without its parent path. The result is the last
 * element of the original SdfPath.
 *
 * @param[in] path is the SdfPath from which to remove the parent path.
 *
 * @return The path without its parent path.
 */
MAYAHYDRALIB_API
pxr::SdfPath MakeRelativeToParentPath(const pxr::SdfPath& path);

/**
 * @brief Get the Hydra Xform matrix from a given prim.
 *
 * @usage Get the Hydra Xform matrix from a given prim. This method makes no guarantee on whether
 * the matrix is flattened or not.
 *
 * @param[in] prim is the Hydra prim in the SceneIndex of which to get the transform matrix.
 * @param[out] outMatrix is the transform matrix of the prim.
 *
 * @return True if the operation succeeded, false otherwise.
 */
MAYAHYDRALIB_API
bool GetXformMatrixFromPrim(const pxr::HdSceneIndexPrim& prim, pxr::GfMatrix4d& outMatrix);

} // namespace MAYAHYDRA_NS_DEF

#endif // MAYAHYDRALIB_HYDRA_UTILS_H
