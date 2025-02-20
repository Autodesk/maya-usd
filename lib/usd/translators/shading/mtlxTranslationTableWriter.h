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

#include "mtlxBaseWriter.h"

#include <mayaUsd/fileio/writeJobContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>

#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

// This is basically UsdMayaSymmetricShaderWriter with a table for attribute renaming:
class MtlxUsd_TranslationTableWriter : public MtlxUsd_BaseWriter
{
public:
    using TranslationTable = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
    using AlwaysAuthored = std::unordered_set<TfToken, TfToken::HashFunctor>;

    MtlxUsd_TranslationTableWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath&           usdPath,
        UsdMayaWriteJobContext&  jobCtx,
        TfToken                  materialName,
        const TranslationTable&  translationTable,
        const AlwaysAuthored&    alwaysAuthored);

    void Write(const UsdTimeCode& usdTime) override;

    UsdAttribute
    GetShadingAttributeForMayaAttrName(const TfToken&, const SdfValueTypeName&) override;

private:
    std::unordered_map<TfToken, MPlug, TfToken::HashFunctor> _inputNameAttrMap;
    TfToken                                                  _materialName;
    const TranslationTable&                                  _translationTable;
    const AlwaysAuthored&                                    _alwaysAuthored;
};

PXR_NAMESPACE_CLOSE_SCOPE
