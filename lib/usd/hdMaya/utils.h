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
#ifndef HDMAYA_UTILS_H
#define HDMAYA_UTILS_H

#include <hdMaya/adapters/mayaAttrs.h>
#include <hdMaya/api.h>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hd/textureResource.h>
#include <pxr/imaging/hd/types.h>
#include <pxr/pxr.h>

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDag.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MRenderUtil.h>
#include <maya/MSelectionList.h>

#include <functional>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

/// \brief Converts a Maya matrix to a double precision GfMatrix.
/// \param mayaMat Maya `MMatrix` to be converted.
/// \return `GfMatrix4d` equal to \p mayaMat.
inline GfMatrix4d GetGfMatrixFromMaya(const MMatrix& mayaMat)
{
    GfMatrix4d mat;
    memcpy(mat.GetArray(), mayaMat[0], sizeof(double) * 16);
    return mat;
}

/// \brief Returns a connected "file" shader object to another shader node's
///  parameter.
/// \param obj Maya shader object.
/// \param paramName Name of the parameter to be inspected for connections on
///  \p node shader object.
/// \return Maya object to a "file" shader node, `MObject::kNullObj` if there is
///  no valid connection.
HDMAYA_API
MObject GetConnectedFileNode(const MObject& obj, const TfToken& paramName);

/// \brief Returns a connected "file" shader node to another shader node's
///  parameter.
/// \param node Maya shader node.
/// \param paramName Name of the parameter to be inspected for connections on
///  \p node shader node.
/// \return Maya object to a "file" shader node, `MObject::kNullObj` if there is
///  no valid connection.
HDMAYA_API
MObject GetConnectedFileNode(const MFnDependencyNode& node, const TfToken& paramName);

/// \brief Returns the texture file path from a "file" shader node.
/// \param fileNode "file" shader node.
/// \return Full path to the texture pointed used by the file node. `<UDIM>`
///  tags are kept intact.
HDMAYA_API
TfToken GetFileTexturePath(const MFnDependencyNode& fileNode);

/// \brief Returns the texture resource from a "file" shader node.
/// \param fileObj "file" shader object.
/// \param filePath Path to the texture file held by "file" shader node.
/// \param maxTextureMemory Maximum texture memory in bytes available for
///  loading the texture. If the texture requires more memory
///  than \p maxTextureMemory, higher mip-map levels are discarded until the
///  memory required is less than \p maxTextureMemory.
/// \return Pointer to the Hydra Texture resource.
HDMAYA_API
HdTextureResourceSharedPtr GetFileTextureResource(
    const MObject& fileObj,
    const TfToken& filePath,
    int            maxTextureMemory = 4 * 1024 * 1024);

/// \brief Returns the texture wrapping parameters from a "file" shader node.
/// \param fileObj "file" shader object.
/// \return A `std::tuple<HdWrap, HdWrap>` holding the wrapping parameters
///  for s and t axis.
HDMAYA_API
std::tuple<HdWrap, HdWrap> GetFileTextureWrappingParams(const MObject& fileObj);

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDMAYA_UTILS_H
