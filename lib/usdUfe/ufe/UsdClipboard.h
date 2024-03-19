#ifndef USDCLIPBOARD_H
#define USDCLIPBOARD_H

//
// Copyright 2024 Autodesk
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

#include <usdUfe/base/api.h>

#include <pxr/usd/usd/common.h>
#include <pxr/usd/usdUtils/stageCache.h>

namespace USDUFE_NS_DEF {

//! \brief Class to handle clipboard USD data.
class USDUFE_PUBLIC UsdClipboard
{
public:
    // Clipboard file name and format.
    static constexpr auto clipboardFileName = "UsdUfeClipboard.usd";
    using Ptr = std::shared_ptr<UsdClipboard>;

    UsdClipboard();
    ~UsdClipboard();

    // Delete the copy/move constructors assignment operators.
    UsdClipboard(const UsdClipboard&) = delete;
    UsdClipboard& operator=(const UsdClipboard&) = delete;
    UsdClipboard(UsdClipboard&&) = delete;
    UsdClipboard& operator=(UsdClipboard&&) = delete;

    //! \brief Set the clipboard data.
    //! \note  It is possible to set clipboard data across multiple running instances of DCC.
    //! \param clipboard The clipboard data to set (aka the clipboard stage).
    void setClipboardData(const PXR_NS::UsdStageWeakPtr& clipboardData);

    //! \brief Get the clipboard data.
    //! \note It is possible to set clipboard data across multiple running instances of the
    //        DCC app (ex: Maya), so we get the last modified clipboard data.
    //! \return The clipboard data (aka the clipboard stage).
    PXR_NS::UsdStageWeakPtr getClipboardData();

    //! \brief Set the clipboard path, i.e. where the .usd should be exported and read from.
    //! \note The filename "UsdUfeClipboard.usd" will be appended to the input path.
    //! \param clipboardPath The new clipboard path.
    void setClipboardPath(const std::string& clipboardPath);

    //! \brief Sets the clipboard path (including filename) where data should exported and
    //! read from.
    //! \param clipboardFilePath The new clipboard path+file.
    void setClipboardFilePath(const std::string& clipboardFilePath);

    //! \brief Sets the USD file format for the clipboard file.
    //! \param formatTag The USD file format to use.
    void setClipboardFileFormat(const std::string& formatTag);

    //! \brief Clean the clipboard data so no paste action will happen.
    void cleanClipboard();

private:
    // The clipboard path (including filename).
    std::string _clipboardFilePath;

    // The USD file format to use for the clipboard file.
    std::string _clipboardFileFormat;

    // The cache clipboard stage id.
    PXR_NS::UsdStageCache::Id _clipboardStageCacheId;

    //! \brief Erase the clipboard stage from the cache.
    void cleanClipboardStageCache();

    //! \brief Remove the clipboard file by deleting it.
    void removeClipboardFile();
};

} // namespace USDUFE_NS_DEF

#endif // USDCLIPBOARD_H
