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
#include "mtlxBaseReader.h"

#include <mayaUsd/fileio/primReaderArgs.h>
#include <mayaUsd/fileio/primReaderContext.h>

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

// A symmetric reader that takes a translation table. Useful when there
// is a 1-to-1 mapping between camelCased and snake_cased names.
class MtlxUsd_TranslationTableReader : public MtlxUsd_BaseReader
{
public:
    MtlxUsd_TranslationTableReader(const UsdMayaPrimReaderArgs&);

    using TranslationTable = std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;

    virtual const TfToken&          getMaterialName() const = 0;
    virtual const TfToken&          getOutputName() const = 0;
    virtual const TranslationTable& getTranslationTable() const = 0;

    bool Read(UsdMayaPrimReaderContext& context) override;

    TfToken GetMayaNameForUsdAttrName(const TfToken& usdAttrName) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE
