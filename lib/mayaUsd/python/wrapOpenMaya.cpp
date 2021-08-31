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

// Hack because MDGModifier copy constructor and = operator are not public.
#include <maya/MApiNamespace.h>
#undef OPENMAYA_PRIVATE
#define OPENMAYA_PRIVATE public

#include <maya/MPlug.h>
#include <maya/MDagPath.h>
#include <maya/MDGModifier.h>

#include <pxr/usd/usdGeom/camera.h>

#include <boost/python.hpp>
#include <boost/python/args.hpp>
#include <boost/python/def.hpp>
#include <boost/python/wrapper.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    // Required because MObject::classname does not exists
    template <class MayaClass> const char* classname() { return MayaClass::className(); }
    template <>                const char* classname<MObject>() { return "MObject"; }

/* Should be detectable detecting maya_useNewAPI in plugin

# Using the Maya Python API 2.0.
def maya_useNewAPI():
    pass
*/

#ifdef PYTHON_API_V1
    typedef void *(*swig_converter_func)(void *, int *);
    typedef struct swig_type_info *(*swig_dycast_func)(void **);
    typedef struct {
        PyObject_HEAD
        void *ptr;
        swig_type_info *ty;
        int own;
        PyObject *next;
        PyObject *dict;
    } SwigPyObject;

    template <class MayaClass>
    struct MayaClassConverter
    {
        // to-python conversion of const MayaClass.
        static PyObject* convert(const MayaClass& object)
        {
            TfPyLock lock;
            boost::python::object classInstance = boost::python::import("maya.OpenMaya").attr(classname<MayaClass>())();
            boost::python::object theSwigObject = classInstance.attr("this");
            SwigPyObject* theSwigPyObject = (SwigPyObject*)theSwigObject.ptr();
            *((MayaClass*)(theSwigPyObject->ptr)) = object;
            return boost::python::incref(classInstance.ptr());
        }
    };
#else
    enum MPyObjectLifetimeMode
    {
        kNone,
        kOwned,
        kShared,
        kWeak
    };

    template <class M>
    struct MPyObject : public PyObject
    {
    public:
        typedef M MayaType;
        M* fPtr;
        MPyObjectLifetimeMode fLifetime;
    };

    template <class MayaClass>
    struct MayaClassConverter
    {
        // to-python conversion of const MayaClass.
        static PyObject* convert(const MayaClass& object)
        {
            TfPyLock lock;
            boost::python::object classInstance = boost::python::import("maya.api.OpenMaya").attr(classname<MayaClass>())();
            MPyObject<MayaClass>* thePyObject = (MPyObject<MayaClass>*)classInstance.ptr();
            *((MayaClass*)(thePyObject->fPtr)) = object;
            return boost::python::incref(classInstance.ptr());
        }
    };
#endif
}

void wrapOpenMaya()
{
    boost::python::to_python_converter<MObject, MayaClassConverter<MObject>>();
    boost::python::to_python_converter<MDagPath, MayaClassConverter<MDagPath>>();
    boost::python::to_python_converter<MPlug, MayaClassConverter<MPlug>>();
    boost::python::to_python_converter<MDGModifier, MayaClassConverter<MDGModifier>>();
}