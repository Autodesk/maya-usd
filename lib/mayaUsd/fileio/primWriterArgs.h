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
#ifndef PXRUSDMAYA_PRIMWRITERERARGS_H
#define PXRUSDMAYA_PRIMWRITERERARGS_H

#include <mayaUsd/base/api.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>

#include <maya/MDagPath.h>
#include <maya/MObject.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdMayaPrimWriterArgs
/// \brief This class holds read-only arguments that are passed into the writer
/// plugins for the usdMaya library.  This mostly contains functions to get data
/// from the maya scene and helpers to retrieve values from maya and prepare
/// them to author into usd.
///
/// \sa UsdMayaPrimWriterContext
class UsdMayaPrimWriterArgs
{
public:
    MAYAUSD_CORE_PUBLIC
    UsdMayaPrimWriterArgs(const MDagPath& dagPath, const bool exportRefsAsInstanceable);

    /// \brief returns the MObject that should be exported.
    MAYAUSD_CORE_PUBLIC
    MObject GetMObject() const;

    MAYAUSD_CORE_PUBLIC
    const MDagPath& GetMDagPath() const;

    MAYAUSD_CORE_PUBLIC
    bool GetExportRefsAsInstanceable() const;

    /// helper functions to get data from attribute named \p from the current
    /// MObject.
    /// \{
    MAYAUSD_CORE_PUBLIC
    bool ReadAttribute(const std::string& name, std::string* val) const;
    MAYAUSD_CORE_PUBLIC
    bool ReadAttribute(const std::string& name, VtIntArray* val) const;
    MAYAUSD_CORE_PUBLIC
    bool ReadAttribute(const std::string& name, VtFloatArray* val) const;
    MAYAUSD_CORE_PUBLIC
    bool ReadAttribute(const std::string& name, VtVec3fArray* val) const;
    /// \}

private:
    MDagPath _dagPath;
    bool     _exportRefsAsInstanceable;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
