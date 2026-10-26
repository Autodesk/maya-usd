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

#include <usdLayerEditor/lib/layerEditorAPI.h>

#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>

#include <string>

namespace UsdLayerEditor {
namespace FileSystem {

/*! \brief returns parent directory of opened DCC scene file
 */
LAYEREDITOR_PUBLIC std::string getDCCSceneFileDir();

/*! \brief returns the DCC workspace file rule entry for scenes
 */
LAYEREDITOR_PUBLIC std::string getDCCWorkspaceScenesDir();

/*! \brief takes in an absolute file path and returns the path relative to dcc scene file.
When there is no scene file, the absolute (input) path will be returned.
 */
LAYEREDITOR_PUBLIC std::string getPathRelativeToDCCSceneFile(const std::string& fileName);

/*! \brief returns the flag specifying whether USD file paths should be saved as relative to Maya
 * scene file
 */
LAYEREDITOR_PUBLIC bool requireUsdPathsRelativeToDCCSceneFile();

/*! \brief prepares the UI used to save layers, so that the UI can potentially make the
           selected file name relative to the given directory.
 */
LAYEREDITOR_PUBLIC bool prepareLayerSaveUILayer(const std::string& relativeAnchor);

/*! \brief prepares the UI used to save layers with the given layer file path, so that the UI
           can potentially make the selected file name relative to that layer. If the layer is
           null, the UI can either use the scene file or not make the file relative.
 */
LAYEREDITOR_PUBLIC bool
prepareLayerSaveUILayer(const PXR_NS::SdfLayerHandle& layer, bool useSceneFileForRoot);

/*! \brief returns the flag specifying whether USD file paths should be saved
 *         as relative to the given parent layer.
 */
LAYEREDITOR_PUBLIC bool requireUsdPathsRelativeToParentLayer();

/*! \brief sets the flag specifying whether USD file paths should be saved as relative to Maya
 * scene file
 */
LAYEREDITOR_PUBLIC void setRequireUsdPathsRelativeToDCCSceneFile(bool value);

/*! \brief sets the flag specifying whether USD file paths should be saved
 *         as relative to the given parent layer.
 */
LAYEREDITOR_PUBLIC void setRequireUsdPathsRelativeToParentLayer(bool value);

/*! \brief returns a unique file name
 */
LAYEREDITOR_PUBLIC std::string
                   getUniqueFileName(const std::string& dir, const std::string& basename, const std::string& ext);

/*! \brief returns a unique file name, make sure it does not exist on disk.
 */
LAYEREDITOR_PUBLIC std::string ensureUniqueFileName(const std::string& filename);

/*! \brief returns the position of the numbered suffix.
           Returns the end of the string position if no such suffix is present.
 */
LAYEREDITOR_PUBLIC size_t getNumberSuffixPosition(const std::string& text);

/*! \brief returns the numbered suffix.
           Returns the an empty string if no such suffix is present.
 */
LAYEREDITOR_PUBLIC std::string getNumberSuffix(const std::string& text);

/*! \brief returns a new text with the numbered suffix increased by one.
           Returns the text with 1 appended if no such suffix is present.
 */
LAYEREDITOR_PUBLIC std::string increaseNumberSuffix(const std::string& text);

/**
 * Checks a file for write access via the DCC registry.
 * @param filePath The file to check for write access.
 * @return True if the file can be written to, false otherwise.
 */
LAYEREDITOR_PUBLIC bool checkWriteAccess(const std::string& filePath);

// Backup a file and restore it if not committed.
class LAYEREDITOR_PUBLIC FileBackup
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
} // namespace UsdLayerEditor
