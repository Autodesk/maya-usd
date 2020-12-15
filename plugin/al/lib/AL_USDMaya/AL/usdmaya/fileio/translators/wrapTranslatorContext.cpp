//
// Copyright 2017 Animal Logic
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
#include "AL/maya/utils/Utils.h"
#include "AL/usdmaya/fileio/translators/TranslatorContext.h"
#include "AL/usdmaya/nodes/ProxyShape.h"

#include <pxr/base/tf/makePyConstructor.h>
#include <pxr/base/tf/pyPtrHelpers.h>

#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MObjectHandle.h>

#include <boost/python.hpp>

using namespace AL::usdmaya::fileio::translators;
using namespace AL::usdmaya::nodes;
using namespace AL::maya::utils;

using namespace boost::python;

namespace {

static std::string _getTransform(TranslatorContext& self, const SdfPath& path)
{
    MObjectHandle handle;
    if (self.getTransform(path, handle)) {
        MFnDagNode fn(handle.object());
        return fn.fullPathName().asChar();
    }
    return {};
}

static std::string _getTransform(TranslatorContext& self, const UsdPrim& prim)
{
    return _getTransform(self, prim.GetPath());
}

static std::string
_getMObjectPathWithTypeId(TranslatorContext& self, const SdfPath& path, uint32_t type)
{
    MObjectHandle handle;
    if (self.getMObject(path, handle, MTypeId(type))) {
        MFnDagNode fn(handle.object());
        return fn.fullPathName().asChar();
    }
    return {};
}

static std::string
_getMObjectPathWithTypeId(TranslatorContext& self, const UsdPrim& prim, uint32_t type)
{
    return _getMObjectPathWithTypeId(self, prim.GetPath(), type);
}

static std::string
_getMObjectPathWithFnType(TranslatorContext& self, const SdfPath& path, uint32_t type)
{
    MObjectHandle handle;
    if (self.getMObject(path, handle, static_cast<MFn::Type>(type))) {
        MFnDagNode fn(handle.object());
        return fn.fullPathName().asChar();
    }
    return {};
}

static std::string
_getMObjectPathWithFnType(TranslatorContext& self, const UsdPrim& prim, uint32_t type)
{
    return _getMObjectPathWithFnType(self, prim.GetPath(), type);
}

static std::vector<std::string> _getMObjectsPath(TranslatorContext& self, const SdfPath& path)
{
    std::vector<std::string> paths;

    MObjectHandleArray array;
    if (!self.getMObjects(path, array)) {
        return {};
    }

    for (const auto& handle : array) {
        MFnDependencyNode fn(handle.object());
        paths.push_back(fn.name().asChar());
    }

    return paths;
}

static std::vector<std::string> _getMObjectsPath(TranslatorContext& self, const UsdPrim& prim)
{
    return _getMObjectsPath(self, prim.GetPath());
}

static void _insertItem(TranslatorContext& self, const UsdPrim& prim, const std::string& path)
{
    self.insertItem(prim, findMayaObject(MString(path.c_str())));
}

} // namespace

//----------------------------------------------------------------------------------------------------------------------
void wrapTranslatorContext()
{
    typedef TfWeakPtr<TranslatorContext> TranslatorContextPtr;

    class_<TranslatorContext, TranslatorContextPtr, boost::noncopyable>(
        "TranslatorContext", no_init)
        .def(TfPyRefAndWeakPtr())
        .def("create", &TranslatorContext::create, return_value_policy<TfPyRefPtrFactory<>>())
        .staticmethod("create")
        .def("getProxyShape", &TranslatorContext::getProxyShape, return_internal_reference<>())
        .def("getUsdStage", &TranslatorContext::getUsdStage)
        .def(
            "getTransformPath",
            (std::string(*)(TranslatorContext&, const UsdPrim&)) & _getTransform,
            (arg("prim")))
        .def(
            "getTransformPath",
            (std::string(*)(TranslatorContext&, const SdfPath&)) & _getTransform,
            (arg("sdfPath")))
        .def(
            "getMObjectPathWithTypeId",
            (std::string(*)(TranslatorContext&, const SdfPath&, uint32_t))
                & _getMObjectPathWithTypeId,
            (arg("sdfPath"), arg("typeId")))
        .def(
            "getMObjectPathWithTypeId",
            (std::string(*)(TranslatorContext&, const UsdPrim&, uint32_t))
                & _getMObjectPathWithTypeId,
            (arg("prim"), arg("typeId")))
        .def(
            "getMObjectPathWithFnType",
            (std::string(*)(TranslatorContext&, const SdfPath&, uint32_t))
                & _getMObjectPathWithFnType,
            (arg("sdfPath"), arg("fnType")))
        .def(
            "getMObjectPathWithFnType",
            (std::string(*)(TranslatorContext&, const UsdPrim&, uint32_t))
                & _getMObjectPathWithFnType,
            (arg("prim"), arg("fnType")))
        .def(
            "getMObjectsPath",
            (std::vector<std::string>(*)(TranslatorContext&, const SdfPath&)) & _getMObjectsPath,
            (arg("sdfPath")))
        .def(
            "getMObjectsPath",
            (std::vector<std::string>(*)(TranslatorContext&, const UsdPrim&)) & _getMObjectsPath,
            (arg("prim")))
        .def("insertItem", &_insertItem);
}

TF_REFPTR_CONST_VOLATILE_GET(TranslatorContext)
