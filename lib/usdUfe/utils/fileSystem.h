//
// Copyright 2026 Autodesk
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
#ifndef USDUFE_UTIL_FILESYSTEM_H
#define USDUFE_UTIL_FILESYSTEM_H

#include <usdUfe/base/api.h>

#include <pxr/usd/sdf/layer.h>

#include <string>
#include <utility>

namespace USDUFE_NS_DEF {
namespace FileSystem {

/*! \brief returns the resolved filesystem path for the file identified by the given path
 */
USDUFE_PUBLIC
std::string resolvePath(const std::string& filePath);

/*! \brief returns the parent directory of the given file path
 */
USDUFE_PUBLIC
std::string getDir(const std::string& fullFilePath);

/*! \brief Takes in two absolute file paths and computes a relative path of the first one
           to second one.

    \return A pair with the path and a boolean that indicates if the attempt to make the
            file name relative to the valid anchor path succeeded.

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
USDUFE_PUBLIC
std::pair<std::string, bool>
makePathRelativeTo(const std::string& fileName, const std::string& relativeToDir);

/*! \brief returns relative path of a layer file to its parent layer's directory
 */
USDUFE_PUBLIC
std::string
getPathRelativeToDirectory(const std::string& fileName, const std::string& relativeToDir);

/*! \brief returns parent directory of the given layer.
 */
USDUFE_PUBLIC
std::string getLayerFileDir(const PXR_NS::SdfLayerHandle& layer);

/*! \brief takes in an absolute file path and returns the path relative to a USD layer.
           When there is no layer or the layer has never been saved, then the absolute
           (input) path will be returned.
 */
USDUFE_PUBLIC
std::string
getPathRelativeToLayerFile(const std::string& fileName, const PXR_NS::SdfLayerHandle& layer);

/*! \brief Marks a certain file path inside the layer to be made relative in a postponed fashion.
The marked file paths will be turned into relative paths upon calling updatePostponedRelativePaths.
 */
USDUFE_PUBLIC
void markPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath);

/*! \brief Marks an asset path attribute of the layer whose value, if its path was marked through
markPathAsPostponedRelative, is turned into a relative path by updatePostponedRelativePaths.
 */
USDUFE_PUBLIC
void markAttributeAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            attrName);

/*! \brief Unmarks file path which was marked through the call to markPathAsPostponedRelative.
 */
USDUFE_PUBLIC
void unmarkPathAsPostponedRelative(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            contentPath);

/*! \brief Turns the file paths marked through the call 'markPathAsPostponedRelative' to relative.
 */
USDUFE_PUBLIC
void updatePostponedRelativePaths(
    const PXR_NS::SdfLayerHandle& layer,
    const std::string&            layerFileName);

/**
 * Appends `b` to the path `a` and returns a path (by appending two input paths).
 *
 * @param a         A string that represents the first path
 * @param b         A string that represents the second path
 *
 * @return         the two paths joined by a separator
 */
USDUFE_PUBLIC
std::string appendPaths(const std::string& a, const std::string& b);

/*! \brief returns a unique file name
 */
USDUFE_PUBLIC
std::string
getUniqueFileName(const std::string& dir, const std::string& basename, const std::string& ext);

/*! \brief returns a unique file name, make sure it does not exist on disk.
 */
USDUFE_PUBLIC
std::string ensureUniqueFileName(const std::string& filename);

/*! \brief returns the position of the numbered suffix.
           Returns the end of the string position if no such suffix is present.
 */
USDUFE_PUBLIC
size_t getNumberSuffixPosition(const std::string& text);

/*! \brief returns the numbered suffix.
           Returns the an empty string if no such suffix is present.
 */
USDUFE_PUBLIC
std::string getNumberSuffix(const std::string& text);

/*! \brief returns a new text with the numbered suffix increased by one.
           Returns the text with 1 appended if no such suffix is present.
 */
USDUFE_PUBLIC
std::string increaseNumberSuffix(const std::string& text);

// Backup a file and restore it if not committed.
class USDUFE_PUBLIC FileBackup
{
public:
    FileBackup(const std::string& filename);
    ~FileBackup();

    // Once committed, the backup will not be put back into the original file.
    void commit();

    // Force restoration of the original file if successfully backed-up, even if committed.
    void restore();

    // Return the backup file name.
    std::string getBackupFilename() const;

public:
    void backup();

    std::string _filename;
    bool        _backed = false;
    bool        _commited = false;
};

} // namespace FileSystem
} // namespace USDUFE_NS_DEF

#endif // USDUFE_UTIL_FILESYSTEM_H
