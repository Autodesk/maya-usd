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
#include "primWriterArgs.h"

#include <mayaUsd/fileio/utils/writeUtil.h>

#include <maya/MFnDependencyNode.h>

PXR_NAMESPACE_OPEN_SCOPE

UsdMayaPrimWriterArgs::UsdMayaPrimWriterArgs(
    const MDagPath& dagPath,
    const bool      exportRefsAsInstanceable)
    : _dagPath(dagPath)
    , _exportRefsAsInstanceable(exportRefsAsInstanceable)
{
}

MObject UsdMayaPrimWriterArgs::GetMObject() const { return _dagPath.node(); }

const MDagPath& UsdMayaPrimWriterArgs::GetMDagPath() const { return _dagPath; }

bool UsdMayaPrimWriterArgs::GetExportRefsAsInstanceable() const
{
    return _exportRefsAsInstanceable;
}

bool UsdMayaPrimWriterArgs::ReadAttribute(const std::string& name, std::string* val) const
{
    return UsdMayaWriteUtil::ReadMayaAttribute(
        MFnDependencyNode(GetMObject()), MString(name.c_str()), val);
}

bool UsdMayaPrimWriterArgs::ReadAttribute(const std::string& name, VtIntArray* val) const
{
    return UsdMayaWriteUtil::ReadMayaAttribute(
        MFnDependencyNode(GetMObject()), MString(name.c_str()), val);
}

bool UsdMayaPrimWriterArgs::ReadAttribute(const std::string& name, VtFloatArray* val) const
{
    return UsdMayaWriteUtil::ReadMayaAttribute(
        MFnDependencyNode(GetMObject()), MString(name.c_str()), val);
}

bool UsdMayaPrimWriterArgs::ReadAttribute(const std::string& name, VtVec3fArray* val) const
{
    return UsdMayaWriteUtil::ReadMayaAttribute(
        MFnDependencyNode(GetMObject()), MString(name.c_str()), val);
}

PXR_NAMESPACE_CLOSE_SCOPE
