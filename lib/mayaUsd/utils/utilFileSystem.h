//
// Copyright 2019 Autodesk
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
#include <mayaUsd/base/api.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>

#include <maya/MObject.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdMayaUtilFileSystem {

/*! \brief returns parent directory of a maya scene file opened by reference
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaReferencedFileDir(const MObject& proxyShapeNode);

/*! \brief returns parent directory of opened maya scene file
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaSceneFileDir();

/*! \brief returns the path of a file relative to the Maya scene project folder.
           Returns an empty string if the path is not relative to the project.
 */
MAYAUSD_CORE_PUBLIC
std::string getPathRelativeToProject(const std::string& fileName);

/*! \brief returns the absolute path of a file but relative to the Maya scene project folder.
           Returns an empty string if the path cannot be made relative to the project.
 */
MAYAUSD_CORE_PUBLIC
std::string makeProjectRelatedPath(const std::string& fileName);

/*! \brief takes in an absolute file path and returns the path relative to maya scene file.
When there is no scene file, the absolute (input) path will be returned.
 */
MAYAUSD_CORE_PUBLIC
std::string getPathRelativeToMayaSceneFile(const std::string& fileName);

/*! \brief Performs handling of a USD asset path attribute that maybe relative to a layer.
Whether the file path is required to be relative is defined by the optionvar which name is supplied
through 'optionVarName' variable. If the relative file path is required and the layer is saved on
disk (not anonymous) then the function returns the appropriate relative path. If the relative file
path is required and the layer is anonymous (not saved on disk) then the function returns the
absolute path and this path is registered to be made relative in a postponed fashion. Otherwise the
function returns the absolute path.
 */
MAYAUSD_CORE_PUBLIC
std::string handleAssetPathThatMaybeRelativeToLayer(
    std::string                   fileName,
    const std::string&            attrName,
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            optionVarName);

/*! \brief returns the flag specifying whether USD file paths should be saved as relative to Maya
 * scene file
 */
MAYAUSD_CORE_PUBLIC
bool requireUsdPathsRelativeToMayaSceneFile();

/*! \brief returns true if the USD file should be added as a reference, false for as a payload.
 */
MAYAUSD_CORE_PUBLIC
bool wantReferenceCompositionArc();

/*! \brief returns true if the USD the reference or payload should be prepend, else append.
 */
MAYAUSD_CORE_PUBLIC
bool wantPrependCompositionArc();

/*! \brief returns true if the USD payload should be immediately loaded.
 */
MAYAUSD_CORE_PUBLIC
bool wantPayloadLoaded();

/*! \brief returns the prim path referenced by the USD reference or payload.
 */
MAYAUSD_CORE_PUBLIC
std::string getReferencedPrimPath();

/*! \brief returns the Maya workspace file rule entry for scenes
 */
MAYAUSD_CORE_PUBLIC std::string getMayaWorkspaceScenesDir();

/*! \brief returns the absolute path relative to the Maya file
 */
MAYAUSD_CORE_PUBLIC
std::string resolveRelativePathWithinMayaContext(
    const MObject&     proxyShape,
    const std::string& relativeFilePath);

/*! \brief returns the resolved filesystem path for the file identified by the given path
 */
MAYAUSD_CORE_PUBLIC
std::string resolvePath(const std::string& filePath);

/*! \brief returns the flag specifying whether USD file paths should be saved
 *         as relative to the current edit target layer.
 */
MAYAUSD_CORE_PUBLIC
bool requireUsdPathsRelativeToEditTargetLayer();

/**
 * Appends `b` to the directory path `a` in-place and inserts directory separators as necessary.
 *
 * @param a         A valid path to a directory on disk. This should be a string
 *                  with a buffer large enough to hold the combined contents of itself and the
 *                  contents of `b`, including the null-terminator.
 * @param b         A string to append as a directory component to `a`.
 *
 * @return          ``true`` if the operation succeeded, ``false`` if an error occurred.
 */
MAYAUSD_CORE_PUBLIC
bool pathAppendPath(std::string& a, const std::string& b);

/**
 * Writes data to a file path on disk.
 *
 * @param filePath      A pointer to the file path to write to on disk.
 * @param buffer        A pointer to the buffer containing the data to write to the file.
 * @param size          The number of bytes to write.
 *
 * @return              The number of bytes written to disk.
 */
MAYAUSD_CORE_PUBLIC
size_t writeToFilePath(const char* filePath, const void* buffer, const size_t size);

/**
 * Removes the path portion of a fully-qualified path and file, in-place.
 *
 * @param filePath      A pointer to the null-terminated ANSI file path to remove the path component
 * for.
 */
MAYAUSD_CORE_PUBLIC
void pathStripPath(std::string& filePath);

MAYAUSD_CORE_PUBLIC
void pathRemoveExtension(std::string& filePath);

MAYAUSD_CORE_PUBLIC
std::string pathFindExtension(std::string& filePath);

/*! \brief turns the file paths marked through the call to
           UsdLayerEditor::FileSystem::markPathAsPostponedRelative to relative, using the real path
           of the layer as the anchor. Does nothing for a null layer.
 */
MAYAUSD_CORE_PUBLIC
void updatePostponedRelativePaths(const PXR_NS::SdfLayerHandle& layer);

/*! \brief returns true if the given child path is inside the given parent directory.
 */
MAYAUSD_CORE_PUBLIC
bool isPathInside(const std::string& parentDir, const std::string& childPath);

} // namespace UsdMayaUtilFileSystem

PXR_NAMESPACE_CLOSE_SCOPE
