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
#ifndef FILE_TEXTURE_UTIL_H
#define FILE_TEXTURE_UTIL_H

/// \file
/// Generic utilities shared between all file texture exporters and importers

#include <pxr/pxr.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

struct FileTextureUtil
{

    /// Computes a USD file name from a Maya file name.
    ///
    /// Makes path relative to \p usdFileName and resolves issues with UDIM naming.
    static void MakeUsdTextureFileName(
        std::string&       fileTextureName,
        const std::string& usdFileName,
        bool               isUDIM);

    /// Computes how many channels a texture file has by loading its header from disk
    static int GetNumberOfChannels(const std::string& fileTextureName);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
