//
// Copyright 2023 Autodesk
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

#include "flexibleSparseValueWriter.h"

PXR_NAMESPACE_OPEN_SCOPE

FlexibleSparseValueWriter::FlexibleSparseValueWriter(bool writeDefaults)
    : _writeDefaults(writeDefaults)
{
}

bool FlexibleSparseValueWriter::SetAttribute(
    const UsdAttribute& attr,
    const VtValue&      value,
    const UsdTimeCode   time)
{
    // If the write-default-values flag is on and the time is the default time,
    // then write the value directly on the attribute, skipping the sparse writer.
    if (_writeDefaults && time.IsDefault()) {
        return attr.Set(value, time);
    } else {
        return _sparseWriter.SetAttribute(attr, value, time);
    }
}

bool FlexibleSparseValueWriter::SetAttribute(
    const UsdAttribute& attr,
    VtValue*            value,
    const UsdTimeCode   time)
{
    // If the write-default-values flag is on and the time is the default time,
    // then write the value directly on the attribute, skipping the sparse writer.
    if (_writeDefaults && time.IsDefault()) {
        return attr.Set(*value, time);
    } else {
        return _sparseWriter.SetAttribute(attr, value, time);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
