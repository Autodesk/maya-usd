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

#ifndef MAYAHYDRA_TEST_UTILS_H
#define MAYAHYDRA_TEST_UTILS_H

#include <pxr/base/gf/matrix4d.h>
#include <pxr/imaging/hd/sceneIndex.h>

#include <maya/MMatrix.h>
#include <maya/MStatus.h>

#include <fstream>
#include <functional>
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

constexpr double DEFAULT_TOLERANCE = std::numeric_limits<double>::epsilon();

using SceneIndicesVector = std::vector<HdSceneIndexBasePtr>;

/**
 * @brief Retrieve the list of registered terminal scene indices
 *
 * @usage Retrieve the list of registered terminal scene indices from the Hydra plugin.
 *
 * @return A reference to the vector of registered terminal scene indices.
 */
const SceneIndicesVector& GetTerminalSceneIndices();

/**
 * @brief Compare a Hydra and a Maya matrix and return whether they are similar
 *
 * @usage Compare a Hydra and a Maya matrix and return whether the difference between each of their
 * corresponding elements is less than or equal to the given tolerance.
 *
 * @param[in] hydraMatrix is the Hydra matrix
 * @param[in] mayaMatrix is the Maya matrix
 * @param[in] tolerance is the maximum allowed difference between two corresponding elements of the
 * matrices. The default value is the epsilon for double precision floating-point numbers.
 *
 * @return True if the two matrices are similar enough given the tolerance, false otherwise.
 */
bool MatricesAreClose(
    const GfMatrix4d& hydraMatrix,
    const MMatrix&    mayaMatrix,
    double            tolerance = DEFAULT_TOLERANCE);

struct PrimEntry
{
    SdfPath          primPath;
    HdSceneIndexPrim prim;
};

using FindPrimPredicate
    = std::function<bool(const HdSceneIndexBasePtr& sceneIndex, const SdfPath& primPath)>;

using PrimEntriesVector = std::vector<PrimEntry>;

class SceneIndexInspector
{
public:
    SceneIndexInspector(HdSceneIndexBasePtr sceneIndex);
    ~SceneIndexInspector();

    /**
     * @brief Retrieve the underlying scene index of this inspector
     *
     * @usage Retrieve the underlying scene index of this inspector. The returned pointer is
     * non-owning.
     *
     * @return A pointer to the underlying scene index of this inspector.
     */
    HdSceneIndexBasePtr GetSceneIndex();

    /**
     * @brief Retrieve all prims that match the given predicate, up until the maximum amount
     *
     * @usage Retrieve all prims that match the given predicate, up until the maximum amount. A
     * maximum amount of 0 means unlimited (all matching prims will be returned).
     *
     * @param[in] predicate is the callable predicate used to determine whether a given prim is
     * desired
     * @param[in] maxPrims is the maximum amount of prims to be retrieved. The default value is 0
     * (unlimited).
     *
     * @return A vector of the prim entries that matched the given predicate.
     */
    PrimEntriesVector FindPrims(FindPrimPredicate predicate, size_t maxPrims = 0) const;

    /**
     * @brief Print the scene index's hierarchy in a tree-like format
     *
     * @usage Print the scene index's hierarchy in a tree-like format, down to the individual data
     * source level.
     *
     * @param[out] outStream is the stream in which to print the hierarchy
     */
    void WriteHierarchy(std::ostream& outStream) const;

private:
    void _FindPrims(
        FindPrimPredicate  predicate,
        const SdfPath&     primPath,
        PrimEntriesVector& primEntries,
        size_t             maxPrims) const;

    void _WritePrimHierarchy(
        SdfPath       primPath,
        std::string   selfPrefix,
        std::string   childrenPrefix,
        std::ostream& outStream) const;

    void _WriteContainerDataSource(
        HdContainerDataSourceHandle dataSource,
        std::string                 dataSourceName,
        std::string                 selfPrefix,
        std::string                 childrenPrefix,
        std::ostream&               outStream) const;

    void _WriteVectorDataSource(
        HdVectorDataSourceHandle dataSource,
        std::string              dataSourceName,
        std::string              selfPrefix,
        std::string              childrenPrefix,
        std::ostream&            outStream) const;

    void _WriteLeafDataSource(
        HdDataSourceBaseHandle dataSource,
        std::string            dataSourceName,
        std::string            selfPrefix,
        std::ostream&          outStream) const;

    HdSceneIndexBasePtr _sceneIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // MAYAHYDRA_TEST_UTILS_H
