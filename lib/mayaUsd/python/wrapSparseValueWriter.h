//
// Copyright 2022 Autodesk
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
#ifndef PYMAYAUSDLIB_SPARSEVALUE_WRITER_H
#define PYMAYAUSDLIB_SPARSEVALUE_WRITER_H

#include <pxr/pxr.h>
#include <pxr/usd/usdUtils/sparseValueWriter.h>

// Python will create copies of classes to prevent dangerous access to ephemeral pointers. This does
// not work well for USD's UsdUtilsSparseValueWriter because it builds up a memory of all previously
// set values, which means it must be passed at least by reference. We will wrap the USD class in a
// way that allows seamless copies in Python land without duplicating the USD class.

PXR_NAMESPACE_USING_DIRECTIVE;

class MayaUsdLibSparseValueWriter
{
    UsdUtilsSparseValueWriter* _writer = nullptr;

public:
    MayaUsdLibSparseValueWriter() = default;

    ~MayaUsdLibSparseValueWriter() = default;

    MayaUsdLibSparseValueWriter(UsdUtilsSparseValueWriter* writer)
        : _writer(writer)
    {
    }

    MayaUsdLibSparseValueWriter(const MayaUsdLibSparseValueWriter& other)
        : _writer(other._writer)
    {
    }

    MayaUsdLibSparseValueWriter& operator=(const MayaUsdLibSparseValueWriter& other)
    {
        // No resource management here. No need to check for this == &other
        _writer = other._writer;
        return *this;
    }

    bool SetAttribute(
        const UsdAttribute& attr,
        const VtValue&      value,
        const UsdTimeCode   time = UsdTimeCode::Default())
    {
        if (_writer) {
            return _writer->SetAttribute(attr, value, time);
        } else {
            return attr.Set(value, time);
        }
    };

    UsdUtilsSparseValueWriter* Get() const { return _writer; }
};

#endif
