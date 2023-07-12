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

#include <mayaHydraLib/api.h>

#include <maya/MDagPath.h>
#include <maya/MMatrix.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

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
