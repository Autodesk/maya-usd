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

/*! \brief returns parent directory of a maya scene file opened by reference
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaReferencedFileDir(const MObject& proxyShapeNode);

/*! \brief returns parent directory of opened maya scene file
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaSceneFileDir();

/*! \brief returns the Maya workspace file rule entry for scenes
 */
MAYAUSD_CORE_PUBLIC
std::string getMayaWorkspaceScenesDir();

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
 * Checks if the given path is a valid directory on disk.
 *
 * @param dirPath   A null-terminated ANSI string directory path to check.
 *
 * @return          ``true`` if the path refers to a valid directory on disk, ``false`` otherwise.
 */
MAYAUSD_CORE_PUBLIC
bool isDirectory(const char* dirPath);

/**
 * Determines if the `filePath` provided is a valid file on disk.
 *
 * @param filePath      A null-terminated ANSI string file path to check.
 *
 * @return              ``true`` if the path refers to a valid file on disk, ``false`` otherwise.
 */
MAYAUSD_CORE_PUBLIC
bool isFile(const char* filePath);

/**
 * Appends `b` to the directory path `a` in-place and inserts directory separators as necessary.
 *
 * @param a         A valid path to a directory on disk. This should be a null-terminated string
 *                  with a buffer large enough to hold the combined contents of itself and the
 *                  contents of `b`, including the null-terminator.
 * @param b         A null-terminated string to append as a directory component to `a`.
 *
 * @return          ``true`` if the operation succeeded, ``false`` if an error occurred.
 */
MAYAUSD_CORE_PUBLIC
bool pathAppendPath(char* a, const char* b);

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
void pathStripPath(char* filePath);

} // namespace UsdMayaUtilFileSystem

PXR_NAMESPACE_CLOSE_SCOPE
