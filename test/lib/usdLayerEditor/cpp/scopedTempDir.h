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
#ifndef MAYAUSD_TEST_USDLAYEREDITOR_SCOPEDTEMPDIR_H
#define MAYAUSD_TEST_USDLAYEREDITOR_SCOPEDTEMPDIR_H

#include <ghc/fs_std.hpp>

#include <string>

namespace UsdLayerEditor {

// Temp directory that cleans itself up, for the tests that need real files on disk.
class ScopedTempDir
{
public:
    explicit ScopedTempDir(const std::string& name)
        : _path(fs::filesystem::temp_directory_path() / name)
    {
        fs::filesystem::create_directories(_path);
    }
    ~ScopedTempDir()
    {
        std::error_code ignored;
        fs::filesystem::remove_all(_path, ignored);
    }

    ScopedTempDir(const ScopedTempDir&) = delete;
    ScopedTempDir& operator=(const ScopedTempDir&) = delete;

    std::string file(const std::string& name) const { return (_path / name).generic_string(); }

    std::string path() const { return _path.generic_string(); }

private:
    fs::filesystem::path _path;
};

} // namespace UsdLayerEditor

#endif // MAYAUSD_TEST_USDLAYEREDITOR_SCOPEDTEMPDIR_H
