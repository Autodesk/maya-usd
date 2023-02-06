//
// Copyright 2019 Luma Pictures
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
/// \file hdmaya/utils.h
///
/// Utilities for Maya to Hydra, including for adapters and delegates.
#ifndef MAYAHYDRALIB_UTILS_H
#define MAYAHYDRALIB_UTILS_H

#include <mayaHydraLib/adapters/mayaAttrs.h>
#include <mayaHydraLib/api.h>
#include <mayaHydraLib/mayaHydra.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MRenderUtil.h>
#include <maya/MSelectionList.h>

#include <string>

// We would like to preserve additional pathway for dag items similarly 
// to what was done inside InsertData from mtoh. Disabled for now
// #define MAYAHYDRA_DEVELOPMENTAL_ALTERNATE_OBJECT_PATHWAY
// #define MAYAHYDRA_DEVELOPMENTAL_NATIVE_SELECTION

namespace MAYAHYDRA_NS_DEF {

using GfMatrix4d = PXR_NS::GfMatrix4d;
using TfToken = PXR_NS::TfToken;
using SdfPath = PXR_NS::SdfPath;
using TfCallContext = PXR_NS::TfCallContext;

/// \brief Converts a Maya matrix to a double precision GfMatrix.
/// \param mayaMat Maya `MMatrix` to be converted.
/// \return `GfMatrix4d` equal to \p mayaMat.
inline GfMatrix4d GetGfMatrixFromMaya(const MMatrix& mayaMat)
{
    GfMatrix4d mat;
    memcpy(mat.GetArray(), mayaMat[0], sizeof(double) * 16);
    return mat;
}

/// \brief Converts a Maya float matrix to a double precision GfMatrix.
/// \param mayaMat Maya `MFloatMatrix` to be converted.
/// \return `GfMatrix4d` equal to \p mayaMat.
inline GfMatrix4d GetGfMatrixFromMaya(const MFloatMatrix& mayaMat)
{
    GfMatrix4d mat;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j)
            mat[i][j] = mayaMat(i, j);
    }
    return mat;
}

/// \brief Returns a connected "file" shader object to another shader node's
///  parameter.
/// \param obj Maya shader object.
/// \param paramName Name of the parameter to be inspected for connections on
///  \p node shader object.
/// \return Maya object to a "file" shader node, `MObject::kNullObj` if there is
///  no valid connection.
MAYAHYDRALIB_API
MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName);

/// \brief Returns a connected "file" shader node to another shader node's
///  parameter.
/// \param node Maya shader node.
/// \param paramName Name of the parameter to be inspected for connections on
///  \p node shader node.
/// \return Maya object to a "file" shader node, `MObject::kNullObj` if there is
///  no valid connection.
MAYAHYDRALIB_API
MObject GetConnectedFileNode(const MFnDependencyNode& node, const TfToken& paramName);

/// \brief Returns the texture file path from a "file" shader node.
/// \param fileNode "file" shader node.
/// \return Full path to the texture pointed used by the file node. `<UDIM>`
///  tags are kept intact.
MAYAHYDRALIB_API
TfToken GetFileTexturePath(const MFnDependencyNode& fileNode);

/// \brief Return in the std::string outValueAsString the VtValue type and value written as text for debugging purpose
/// \param val the VtValue to be converted.
/// \param outValueAsString the std::string that will contain the text from the VtValue.
MAYAHYDRALIB_API
void ConvertVtValueAsText(const PXR_INTERNAL_NS::VtValue& val, std::string& outValueAsString);

/// \brief Runs a function on all recursive descendents of a selection list
///  May optionally filter by node type. The items in the list are also included
///  in the set of items that are iterated over (assuming they pass the filter).
template <typename FUNC>
inline void
MapSelectionDescendents(const MSelectionList& sel, FUNC func, MFn::Type filterType = MFn::kInvalid)
{
    MStatus  status;
    MItDag   itDag;
    MDagPath currentSelDag;
    MDagPath currentDescendentDag;
    for (MItSelectionList itSel(sel); !itSel.isDone(); itSel.next()) {
        if (itSel.itemType() != MItSelectionList::kDagSelectionItem) {
            continue;
        }
        if (!itSel.getDagPath(currentSelDag)) {
            // our check against itemType means that we should always
            // succeed in getting the dag path, so warn if we don't
            TF_WARN("Error getting dag path from selection");
            continue;
        }

        // We make sure that no parent of the selected item is
        // also selected - otherwise, we would end up re-traversing the
        // same subtree
        bool     parentSelected = false;
        MDagPath parentDag = currentSelDag;
        parentDag.pop();
        for (; parentDag.length() > 0; parentDag.pop()) {
            if (sel.hasItem(parentDag)) {
                parentSelected = true;
                break;
            }
        }
        if (parentSelected) {
            continue;
        }

        // Now we iterate through all dag descendents of the current
        // selected item
        itDag.reset(currentSelDag, MItDag::kDepthFirst, filterType);
        for (; !itDag.isDone(); itDag.next()) {
            status = itDag.getPath(currentDescendentDag);
            if (!status) {
                // just to print an error
                CHECK_MSTATUS(status);
                continue;
            }
            func(currentDescendentDag);
        }
    }
}

MAYAHYDRALIB_API
std::string SanitizeName(const std::string& name);

/// Converts the given Maya MDagPath \p dagPath into an SdfPath.
///
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAHYDRALIB_API
SdfPath DagPathToSdfPath(
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
MAYAHYDRALIB_API
SdfPath RenderItemToSdfPath(
    const MRenderItem& ri,
    const bool         mergeTransformAndShape,
    const bool         stripNamespaces);

} // namespace MAYAHYDRA_NS_DEF

#endif // MAYAHYDRALIB_UTILS_H
