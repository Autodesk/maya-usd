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
/*! \brief returns the resolved filesystem path for the file identified by the given path
 */
MAYAUSD_CORE_PUBLIC
std::string resolvePath(const std::string& filePath);

/*! \brief returns the path to the
 */
MAYAUSD_CORE_PUBLIC
std::string getDir(const std::string& fullFilePath);

/*! \brief Temporarily change the process current directory.
 *         If built with an empty string, the current directory will not be changed.
 */
class TemporaryCurrentDir
{
public:
    MAYAUSD_CORE_PUBLIC
    TemporaryCurrentDir(const std::string& newCurDir);

    MAYAUSD_CORE_PUBLIC
    ~TemporaryCurrentDir();

    // Restore the previous current directory immediately.
    MAYAUSD_CORE_PUBLIC
    void restore();

private:
    std::string _previousCurDir;
};

/*! \brief Takes in two absolute file paths and computes a relative path of the first one
           to second one.

    \return A pair with the path and a boolean that indicates if the attempt to make the
            file name relative to the valid anchor path failed.

           If the anchor relative-to-directory is empty, then the original file name
           is returned but no failure is returned. If the caller needs to detect
           this as a failure case, they can verify that the relative-to directory
           name is empty themselves before calling this function.

           The rationale for this is that, for example, we don't want to flag as
           an error when the user tries to make a path relative to the scene when
           the scene has not yet been saved.

           If the second path is not absolute or is not reachable from the first,
           then the returned path will still be absolute.
 */
MAYAUSD_CORE_PUBLIC
std::pair<std::string, bool>
makePathRelativeTo(const std::string& fileName, const std::string& relativeToDir);

/*! \brief returns parent directory of a maya scene file opened by reference
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaReferencedFileDir(const MObject& proxyShapeNode);

/*! \brief returns parent directory of opened maya scene file
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaSceneFileDir();

/*! \brief returns parent directory of the given layer.
 */
MAYAUSD_CORE_PUBLIC
std::string getLayerFileDir(const PXR_NS::SdfLayerHandle& layer);

/*! \brief returns the Maya workspace file rule entry for scenes
 */
MAYAUSD_CORE_PUBLIC std::string getMayaWorkspaceScenesDir();

/*! \brief takes in an absolute file path and returns the path relative to maya scene file.
When there is no scene file, the absolute (input) path will be returned.
 */
MAYAUSD_CORE_PUBLIC
std::string getPathRelativeToMayaSceneFile(const std::string& fileName);

/*! \brief takes in an absolute file path and returns the path relative to a USD layer.
           When there is no layer or the layer has never been saved, then the absolute
           (input) path will be returned.
 */
MAYAUSD_CORE_PUBLIC
std::string
getPathRelativeToLayerFile(const std::string& fileName, const PXR_NS::SdfLayerHandle& layer);

/*! \brief returns the flag specifying whether USD file paths should be saved as relative to Maya
 * scene file
 */
MAYAUSD_CORE_PUBLIC
bool requireUsdPathsRelativeToMayaSceneFile();

/*! \brief prepares the UI used to save layers with the given layer file path, so that the UI
           can potentially make the selected file name relative to that layer. If the layer is
           null, the UI can either use the scene file or not make the file relative.
 */
MAYAUSD_CORE_PUBLIC
bool prepareLayerSaveUILayer(const PXR_NS::SdfLayerHandle& layer, bool useSceneFileForRoot);

/*! \brief returns the flag specifying whether USD file paths should be saved
 *         as relative to the given parent layer.
 */
MAYAUSD_CORE_PUBLIC
bool requireUsdPathsRelativeToParentLayer();

/*! \brief returns the flag specifying whether USD file paths should be saved
 *         as relative to the current edit target layer.
 */
MAYAUSD_CORE_PUBLIC
bool requireUsdPathsRelativeToEditTargetLayer();

/*! \brief returns a unique file name
 */
MAYAUSD_CORE_PUBLIC
std::string
getUniqueFileName(const std::string& dir, const std::string& basename, const std::string& ext);

/*! \brief returns the aboluste path relative to the maya file
 */
MAYAUSD_CORE_PUBLIC
std::string resolveRelativePathWithinMayaContext(
    const MObject&     proxyShape,
    const std::string& relativeFilePath);

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
 * Appends `b` to the path `a` and returns a path (by appending two input paths).
 *
 * @param a         A string that respresents the first path
 * @param b         A string that respresents the second path
 *
 * @return         the two paths joined by a seperator
 */
MAYAUSD_CORE_PUBLIC
std::string appendPaths(const std::string& a, const std::string& b);

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

} // namespace UsdMayaUtilFileSystem

PXR_NAMESPACE_CLOSE_SCOPE
