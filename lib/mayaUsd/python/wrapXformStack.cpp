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
#include <mayaUsd/fileio/utils/xformStack.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyEnum.h>
#include <pxr/base/tf/pyResultConversions.h>
#include <pxr/base/tf/stringUtils.h>
#include <pxr/pxr.h>
#include <pxr_python.h>

#include <maya/MEulerRotation.h>
#include <maya/MTransformationMatrix.h>

#include <mutex>

using PXR_BOOST_PYTHON_NAMESPACE::class_;
using PXR_BOOST_PYTHON_NAMESPACE::copy_const_reference;
using PXR_BOOST_PYTHON_NAMESPACE::extract;
using PXR_BOOST_PYTHON_NAMESPACE::no_init;
using PXR_BOOST_PYTHON_NAMESPACE::object;
using PXR_BOOST_PYTHON_NAMESPACE::reference_existing_object;
using PXR_BOOST_PYTHON_NAMESPACE::return_by_value;
using PXR_BOOST_PYTHON_NAMESPACE::return_value_policy;
using PXR_BOOST_PYTHON_NAMESPACE::self;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
// We wrap this class, instead of UsdMayaXformOpClassification directly, mostly
// just so we can handle the .IsNull() -> None conversion
class _PyXformOpClassification
{
public:
    _PyXformOpClassification(const UsdMayaXformOpClassification& opClass)
        : _opClass(opClass)
    {
    }

    bool operator==(const _PyXformOpClassification& other) { return _opClass == other._opClass; }

    TfToken const& GetName() const { return _opClass.GetName(); }

    // In order to return wrapped UsdGeomXformOp::Type objects, we need to import
    // the UsdGeom module
    static void ImportUsdGeomOnce()
    {
        static std::once_flag once;
        std::call_once(once, []() { PXR_BOOST_PYTHON_NAMESPACE::import("pxr.UsdGeom"); });
    }

    UsdGeomXformOp::Type GetOpType() const
    {
        ImportUsdGeomOnce();
        return _opClass.GetOpType();
    }

    bool IsInvertedTwin() const { return _opClass.IsInvertedTwin(); }

    bool IsCompatibleType(UsdGeomXformOp::Type otherType) const
    {
        return _opClass.IsCompatibleType(otherType);
    }

    std::vector<TfToken> CompatibleAttrNames() const { return _opClass.CompatibleAttrNames(); }

    // to-python conversion of const UsdMayaXformOpClassification.
    static PyObject* convert(const UsdMayaXformOpClassification& opClass)
    {
        TfPyLock lock;
        if (opClass.IsNull()) {
            return PXR_BOOST_PYTHON_NAMESPACE::incref(Py_None);
        } else {
            object obj((_PyXformOpClassification(opClass)));
            // Incref because ~object does a decref
            return PXR_BOOST_PYTHON_NAMESPACE::incref(obj.ptr());
        }
    }

private:
    UsdMayaXformOpClassification _opClass;
};

class _PyXformStack
{
public:
    static inline object convert_index(size_t index)
    {
        if (index == UsdMayaXformStack::NO_INDEX) {
            return object(); // return None (handles the incref)
        }
        return object(index);
    }

    // Don't want to make this into a generic conversion rule, via
    //    to_python_converter<UsdMayaXformStack::IndexPair, _PyXformStack>(),
    // because that would make this apply to ANY pair of unsigned ints, which
    // could be dangerous
    static object convert_index_pair(const UsdMayaXformStack::IndexPair& indexPair)
    {
        return PXR_BOOST_PYTHON_NAMESPACE::make_tuple(
            convert_index(indexPair.first), convert_index(indexPair.second));
    }

    static PyObject* convert(const UsdMayaXformStack::OpClassPair& opPair)
    {
        PXR_BOOST_PYTHON_NAMESPACE::tuple result
            = PXR_BOOST_PYTHON_NAMESPACE::make_tuple(opPair.first, opPair.second);
        // Incref because ~object does a decref
        return PXR_BOOST_PYTHON_NAMESPACE::incref(result.ptr());
    }

    static const UsdMayaXformOpClassification& getitem(const UsdMayaXformStack& stack, long index)
    {
        auto raise_index_error = []() {
            PyErr_SetString(PyExc_IndexError, "index out of range");
            PXR_BOOST_PYTHON_NAMESPACE::throw_error_already_set();
        };

        if (index < 0) {
            if (static_cast<size_t>(-index) > stack.GetSize())
                raise_index_error();
            return stack[stack.GetSize() + index];
        } else {
            if (static_cast<size_t>(index) >= stack.GetSize())
                raise_index_error();
            return stack[index];
        }
    }

    static object GetInversionTwins(const UsdMayaXformStack& stack)
    {
        PXR_BOOST_PYTHON_NAMESPACE::list result;
        for (const auto& idxPair : stack.GetInversionTwins()) {
            result.append(convert_index_pair(idxPair));
        }
        return std::move(result);
    }

    static object
    FindOpIndex(const UsdMayaXformStack& stack, const TfToken& opName, bool isInvertedTwin = false)
    {
        return convert_index(stack.FindOpIndex(opName, isInvertedTwin));
    }

    static object FindOpIndexPair(const UsdMayaXformStack& stack, const TfToken& opName)
    {
        return convert_index_pair(stack.FindOpIndexPair(opName));
    }
};
} // namespace

void wrapXformStack()
{
    class_<_PyXformOpClassification>("XformOpClassification", no_init)
        .def(self == self)
        .def("GetName", &_PyXformOpClassification::GetName, return_value_policy<return_by_value>())
        .def("GetOpType", &_PyXformOpClassification::GetOpType)
        .def("IsInvertedTwin", &_PyXformOpClassification::IsInvertedTwin)
        .def("IsCompatibleType", &_PyXformOpClassification::IsCompatibleType)
        .def("CompatibleAttrNames", &_PyXformOpClassification::CompatibleAttrNames);

    PXR_BOOST_PYTHON_NAMESPACE::
        to_python_converter<UsdMayaXformOpClassification, _PyXformOpClassification>();

    class_<UsdMayaXformStack>("XformStack", no_init)
        .def("GetOps", &UsdMayaXformStack::GetOps, return_value_policy<TfPySequenceToList>())
        .def("GetInversionTwins", &_PyXformStack::GetInversionTwins)
        .def("GetNameMatters", &UsdMayaXformStack::GetNameMatters)
        .def("__getitem__", &_PyXformStack::getitem, return_value_policy<return_by_value>())
        .def("__len__", &UsdMayaXformStack::GetSize)
        .def("GetSize", &UsdMayaXformStack::GetSize)
        .def(
            "FindOpIndex",
            &_PyXformStack::FindOpIndex,
            (PXR_BOOST_PYTHON_NAMESPACE::arg("opName"),
             PXR_BOOST_PYTHON_NAMESPACE::arg("isInvertedTwin") = false))
        .def(
            "FindOp",
            &UsdMayaXformStack::FindOp,
            (PXR_BOOST_PYTHON_NAMESPACE::arg("opName"),
             PXR_BOOST_PYTHON_NAMESPACE::arg("isInvertedTwin") = false),
            return_value_policy<copy_const_reference>())
        .def("FindOpIndexPair", &_PyXformStack::FindOpIndexPair)
        .def("FindOpPair", &UsdMayaXformStack::FindOpPair)
        .def("MatchingSubstack", &UsdMayaXformStack::MatchingSubstack)
        .def("MayaStack", &UsdMayaXformStack::MayaStack, return_value_policy<return_by_value>())
        .staticmethod("MayaStack")
        .def("CommonStack", &UsdMayaXformStack::CommonStack, return_value_policy<return_by_value>())
        .staticmethod("CommonStack")
        .def("MatrixStack", &UsdMayaXformStack::MatrixStack, return_value_policy<return_by_value>())
        .staticmethod("MatrixStack")
        .def("FirstMatchingSubstack", &UsdMayaXformStack::FirstMatchingSubstack)
        .staticmethod("FirstMatchingSubstack");

    PXR_BOOST_PYTHON_NAMESPACE::
        to_python_converter<UsdMayaXformStack::OpClassPair, _PyXformStack>();

    PXR_BOOST_PYTHON_NAMESPACE::to_python_converter<
        std::vector<UsdMayaXformStack::OpClass>,
        TfPySequenceToPython<std::vector<UsdMayaXformStack::OpClass>>>();

    TfPyContainerConversions::from_python_sequence<
        std::vector<UsdMayaXformStack const*>,
        TfPyContainerConversions::variable_capacity_policy>();
}
