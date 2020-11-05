//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_IMPORT_TRANSLATOR_H
#define PXRUSDMAYA_IMPORT_TRANSLATOR_H

/// \file usdMaya/importTranslator.h

#include "usdMaya/api.h"

#include <mayaUsd/fileio/jobs/jobArgs.h>

#include <pxr/pxr.h>

#include <maya/MFileObject.h>
#include <maya/MPxFileTranslator.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// File translator for USD files. Handles the USD option in the Import window.
class UsdMayaImportTranslator : public MPxFileTranslator
{
public:
    PXRUSDMAYA_API
    static void* creator();

    PXRUSDMAYA_API
    MStatus reader(
        const MFileObject&                file,
        const MString&                    optionsString,
        MPxFileTranslator::FileAccessMode mode) override;

    bool haveReadMethod() const override { return true; }
    bool haveWriteMethod() const override { return false; }

    PXRUSDMAYA_API
    MFileKind identifyFile(const MFileObject& file, const char* buffer, short size) const override;

    MString defaultExtension() const override
    {
        return UsdMayaTranslatorTokens->UsdFileExtensionDefault.GetText();
    }
    MString filter() const override
    {
        return UsdMayaTranslatorTokens->UsdReadableFileFilter.GetText();
    }

    PXRUSDMAYA_API
    static const std::string& GetDefaultOptions();

private:
    UsdMayaImportTranslator();
    UsdMayaImportTranslator(const UsdMayaImportTranslator&);
    ~UsdMayaImportTranslator() override;
    UsdMayaImportTranslator& operator=(const UsdMayaImportTranslator&);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
