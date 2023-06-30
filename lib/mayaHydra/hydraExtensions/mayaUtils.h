//
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

#ifndef MAYAHYDRALIB_MAYA_UTILS_H
#define MAYAHYDRALIB_MAYA_UTILS_H

#include <mayaHydraLib/api.h>
#include <mayaHydraLib/mayaHydra.h>

#include <maya/MApiNamespace.h>

namespace MAYAHYDRA_NS_DEF {

/**
 * @brief Get the DAG path of a node from its name
 *
 * @usage Get the DAG path of a node from the Maya scene graph using its name.
 *
 * @param[in] nodeName is the name of the node to get the DAG path of.
 * @param[out] outDagPath is the DAG path of the node in the Maya scene graph.
 *
 * @return The resulting status of the operation.
 */
MAYAHYDRALIB_API
MStatus GetDagPathFromNodeName(const MString& nodeName, MDagPath& outDagPath);

/**
 * @brief Get the Maya transform matrix of a node from its DAG path
 *
 * @usage Get the Maya transform matrix of a node from its DAG path. The output transform matrix
 * is the resultant ("flattened") matrix from it and its parents' transforms.
 *
 * @param[in] dagPath is the DAG path of the node in the Maya scene graph.
 * @param[out] outMatrix is the Maya transform matrix of the node.
 *
 * @return The resulting status of the operation.
 */
MAYAHYDRALIB_API
MStatus GetMayaMatrixFromDagPath(const MDagPath& dagPath, MMatrix& outMatrix);

/**
 * @brief Determines whether a given DAG path points to a UFE item created by maya-usd
 *
 * @usage Determines whether a given DAG path points to a UFE item created by maya-usd. UFE stands
 * for Universal Front End : the goal of the Universal Front End is to create a DCC-agnostic
 * component that will allow a DCC to browse and edit data in multiple data models.
 *
 * @param[in] dagPath is the DAG path of the node in the Maya scene graph.
 * @param[out] returnStatus is an optional output variable to return whether the operation was
 * successful. Default value is nullptr (not going to store the result status).
 *
 * @return True if the item pointed to by dagPath is a UFE item created by maya-usd, false
 * otherwise.
 */
MAYAHYDRALIB_API
bool IsUfeItemFromMayaUsd(const MDagPath& dagPath, MStatus* returnStatus = nullptr);

/**
 * @brief Determines whether a given object is a UFE item created by maya-usd
 *
 * @usage Determines whether a given object is a UFE item created by maya-usd. UFE stands
 * for Universal Front End : the goal of the Universal Front End is to create a DCC-agnostic
 * component that will allow a DCC to browse and edit data in multiple data models.
 *
 * @param[in] obj is the object representing the DAG node.
 * @param[out] returnStatus is an optional output variable to return whether the operation was
 * successful. Default value is nullptr (not going to store the result status).
 *
 * @return True if the item represented by obj is a UFE item created by maya-usd, false
 * otherwise.
 */
MAYAHYDRALIB_API
bool IsUfeItemFromMayaUsd(const MObject& obj, MStatus* returnStatus = nullptr);

} // namespace MAYAHYDRA_NS_DEF

#endif // MAYAHYDRALIB_MAYA_UTILS_H
