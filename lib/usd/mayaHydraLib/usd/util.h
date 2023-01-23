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
#ifndef PXRUSDMAYA_UTIL_H
#define PXRUSDMAYA_UTIL_H

#include <pxr/base/tf/token.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/path.h>

#include <maya/MDagPath.h>

#include <string>

#undef MAYAUSD_CORE_PUBLIC
#define MAYAUSD_CORE_PUBLIC

PXR_NAMESPACE_OPEN_SCOPE

/// General utilities for working with the Maya API.
namespace UsdMayaUtil {

/// This is the delimiter that Maya uses to identify levels of hierarchy in the
/// Maya DAG.
const std::string MayaDagDelimiter("|");

/// This is the delimiter that Maya uses to separate levels of namespace in
/// Maya node names.
const std::string MayaNamespaceDelimiter(":");

/// Strip \p nsDepth namespaces from \p nodeName.
///
/// This will turn "taco:foo:bar" into "foo:bar" for \p nsDepth == 1, or
/// "taco:foo:bar" into "bar" for \p nsDepth > 1.
/// If \p nsDepth is -1, all namespaces are stripped.
MAYAUSD_CORE_PUBLIC
std::string stripNamespaces(const std::string& nodeName, const int nsDepth = -1);

MAYAUSD_CORE_PUBLIC
std::string SanitizeName(const std::string& name);

/// Converts the given Maya node name \p nodeName into an SdfPath.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath MayaNodeNameToSdfPath(const std::string& nodeName, const bool stripNamespaces);

/// Converts the given Maya MDagPath \p dagPath into an SdfPath.
///
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath MDagPathToUsdPath(
    const MDagPath& dagPath,
    const bool      mergeTransformAndShape,
    const bool      stripNamespaces);

/// Converts the given Maya MDagPath \p dagPath into an SdfPath.
///
/// If \p mergeTransformAndShape and the dagPath is a shapeNode, it will return
/// the same value as MDagPathToUsdPath(transformPath) where transformPath is
/// the MDagPath for \p dagPath's transform node.
///
/// Elements of the path will be sanitized such that it is a valid SdfPath.
/// This means it will replace Maya's namespace delimiter (':') with
/// underscores ('_').
MAYAUSD_CORE_PUBLIC
SdfPath RenderItemToUsdPath(
	const MRenderItem& ri,
	const bool      mergeTransformAndShape,
	const bool      stripNamespaces);

} // namespace UsdMayaUtil

PXR_NAMESPACE_CLOSE_SCOPE

#endif
