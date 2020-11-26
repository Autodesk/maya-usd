//
// Copyright 2017 Pixar
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
#include <mayaUsd/fileio/utils/meshWriteUtils.h>
#include <mayaUsd/utils/util.h>

#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/array.h>
#include <pxr/pxr.h>

#include <maya/MFnMesh.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

#include <boost/python.hpp>
#include <boost/python/class.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static tuple _GetMeshNormals(const std::string& meshDagPath)
{
    VtArray<GfVec3f> normalsArray;
    TfToken          interpolation;

    MObject meshObj;
    MStatus status = UsdMayaUtil::GetMObjectByName(meshDagPath, meshObj);
    if (status != MS::kSuccess) {
        TF_CODING_ERROR("Could not get MObject for dagPath: %s", meshDagPath.c_str());
        return make_tuple(normalsArray, interpolation);
    }

    MFnMesh meshFn(meshObj, &status);
    if (!meshObj.hasFn(MFn::kMesh)) {
        TF_CODING_ERROR("MFnMesh() failed for object at dagPath: %s", meshDagPath.c_str());
        return make_tuple(normalsArray, interpolation);
    }

    UsdMayaMeshWriteUtils::getMeshNormals(meshObj, &normalsArray, &interpolation);

    return make_tuple(normalsArray, interpolation);
}

// Dummy class for putting UsdMayaMeshWriteUtils namespace functions in a Python
// MeshWriteUtils namespace.
class DummyScopeClass
{
};

} // anonymous namespace

void wrapMeshWriteUtils()
{
    scope s = class_<DummyScopeClass>("MeshWriteUtils", no_init)

                  .def("GetMeshNormals", &_GetMeshNormals)
                  .staticmethod("GetMeshNormals")

        ;
}
