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
#include "exportChaser.h"

PXR_NAMESPACE_OPEN_SCOPE

bool UsdMayaExportChaser::ExportDefault()
{
    // Do nothing by default.
    return true;
}

bool UsdMayaExportChaser::ExportFrame(const UsdTimeCode& time)
{
    // Do nothing by default.
    return true;
}

bool UsdMayaExportChaser::PostExport()
{
    // Do nothing by default.
    return true;
}

void UsdMayaExportChaser::RegisterExtraPrimsPaths(const std::vector<SdfPath>& extraPrimPaths)
{
    _extraPrimsPaths.insert(_extraPrimsPaths.end(), extraPrimPaths.begin(), extraPrimPaths.end());
}

const std::vector<SdfPath>& UsdMayaExportChaser::GetExtraPrimsPaths() const
{
    return _extraPrimsPaths;
}

PXR_NAMESPACE_CLOSE_SCOPE
