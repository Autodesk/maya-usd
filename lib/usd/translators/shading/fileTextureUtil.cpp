//
// Copyright 2021 Autodesk
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
#include "fileTextureUtil.h"

#include "shadingTokens.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/token.h>
#include <pxr/imaging/hio/image.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <ghc/filesystem.hpp>

#include <regex>
#include <system_error>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
// Match UDIM pattern, from 1001 to 1999
const std::regex
    _udimRegex(".*[^\\d](1(?:[0-9][0-9][1-9]|[1-9][1-9]0|0[1-9]0|[1-9]00))(?:[^\\d].*|$)");
} // namespace

void FileTextureUtil::MakeUsdTextureFileName(
    std::string&       fileTextureName,
    const std::string& usdFileName,
    bool               isUDIM)
{
    // WARNING: This extremely minimal attempt at making the file path relative
    //          to the USD stage is a stopgap measure intended to provide
    //          minimal interop. It will be replaced by proper use of Maya and
    //          USD asset resolvers. For package files, the exporter needs full
    //          paths.

    TfToken fileExt(TfGetExtension(usdFileName));
    if (fileExt != UsdMayaTranslatorTokens->UsdFileExtensionPackage) {
        ghc::filesystem::path usdDir(usdFileName);
        usdDir = usdDir.parent_path();
        std::error_code       ec;
        ghc::filesystem::path relativePath = ghc::filesystem::relative(fileTextureName, usdDir, ec);
        if (!ec && !relativePath.empty()) {
            fileTextureName = relativePath.generic_string();
        }
    }

    // Update filename in case of UDIM
    if (isUDIM) {
        std::smatch match;
        if (std::regex_search(fileTextureName, match, _udimRegex) && match.size() == 2) {
            fileTextureName = std::string(match[0].first, match[1].first)
                + TrMayaTokens->UDIMTag.GetString() + std::string(match[1].second, match[0].second);
        }
    }
}

int FileTextureUtil::GetNumberOfChannels(const std::string& fileTextureName)
{
    // Using Hio because the Maya texture node does not provide the information:
    HioImageSharedPtr image = HioImage::OpenForReading(fileTextureName.c_str());

    HioFormat imageFormat = image ? image->GetFormat() : HioFormat::HioFormatUNorm8Vec4;

    // In case of unknown, use 4 channel image:
    if (imageFormat == HioFormat::HioFormatInvalid) {
        imageFormat = HioFormat::HioFormatUNorm8Vec4;
    }
    return HioGetComponentCount(imageFormat);
}

PXR_NAMESPACE_CLOSE_SCOPE
