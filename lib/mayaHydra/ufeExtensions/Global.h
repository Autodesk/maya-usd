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
/// \file hdmaya/utils.h
///
/// Utilities for Maya to Hydra, including for adapters and delegates.
#ifndef UFEEXTENSIONS_GLOBALS_H
#define UFEEXTENSIONS_GLOBALS_H

#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>
#include <ufe/path.h>
#include <ufe/pathSegment.h>
#include <ufe/rtid.h>

#include <ufeExtensions/api.h>

namespace UfeExtensions {

using SdfPath = PXR_NS::SdfPath;

UFEEXTENSIONS_API
Ufe::Rtid getMayaRunTimeId();

//! Convert a single-segment Maya UFE path to a Dag path.  If the argument path
//! is not for a Maya object, or if it has more than one segment, returns an
//! invalid MDagPath.
UFEEXTENSIONS_API
MDagPath ufeToDagPath(const Ufe::Path& ufePath);

UFEEXTENSIONS_API
Ufe::PathSegment sdfPathToUfePathSegment(const SdfPath& usdPath, Ufe::Rtid rtid);

UFEEXTENSIONS_API
Ufe::PathSegment dagPathToUfePathSegment(const MDagPath& dagPath);

} // namespace UfeExtensions

#endif // MAYAHYDRALIB_UTILS_H
