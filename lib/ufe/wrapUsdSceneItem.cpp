//
// Copyright 2019 Autodesk
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



// ***** NOTE *****
//
// This is a WIP that is currently not used (as it doesn't work). We need to
// figure out how to create python bindings for our UsdSceneItem such that it
// derives from the Ufe::SceneItem python bindings.
//


#include "UsdSceneItem.h"

#if 1

#include <boost/python.hpp>

using namespace boost::python;
using namespace MayaUsd::ufe;

struct ufe_SceneItem
{
	static void RegisterConversions()
	{
		// From Python
		converter::registry::push_back(
			&convertible,
			&construct,
			type_id<Ufe::SceneItem::Ptr>());
	}

	// Determine if obj_ptr can be converted into a Ufe::SceneItem
	static void* convertible(PyObject* obj_ptr)
	{
		// Can always put a python object into Ufe::SceneItem
		if (0 == std::strcmp(obj_ptr->ob_type->tp_name, "ufe.PyUfe.SceneItem"))
			return obj_ptr;

		// This causes infinite loop
//		return extract<Ufe::SceneItem::Ptr>(obj_ptr).check() ? obj_ptr : nullptr;

		// This causes infinite loop
// 		extract<Ufe::SceneItem::Ptr> extractor(obj_ptr);
// 		return extractor.check() ? obj_ptr : nullptr;
	}

	// Convert obj_ptr into a SceneItem
	static void construct(
		PyObject* obj_ptr,
		converter::rvalue_from_python_stage1_data* data)
	{
		void* storage = ((converter::rvalue_from_python_storage<Ufe::SceneItem>*)data)->storage.bytes;

		// In-place construct the new SceneItem using the data extracted from the python object.
		extract<UsdSceneItem*> item(obj_ptr);

		// TEST
		//Ufe::SceneItem::Ptr* ptr = static_cast<Ufe::SceneItem::Ptr*>(data->convertible);

		new (storage) Ufe::SceneItem::Ptr(item());
		data->convertible = storage;


// 		// Grab pointer to memory into which to construct the new Ufe::SceneItem.
// 		void* storage = ((converter::rvalue_from_python_storage<Ufe::SceneItem>*)data)->storage.bytes;
// 
// 		// In-place construct the new SceneItem using the data extracted from the python object.
// 		Ufe::SceneItem::Ptr item = extract<Ufe::SceneItem::Ptr>(obj_ptr);

// 		void* storage = ((converter::rvalue_from_python_storage<Map>*)data)->storage.bytes;
// 		new (storage)Map();
// 		data->convertible = storage;

// 		void* storage = ((converter::rvalue_from_python_storage<Usd_PyPrimRange>*)data)->storage.bytes;
// 		Usd_PyPrimRange pyIter = extract<Usd_PyPrimRange>(obj_ptr);
// 		new (storage) UsdPrimRange(pyIter._rng);
// 		data->convertible = storage;
	}
// 	static void
// 	_construct(PyObject *obj_ptr,
// 				converter::rvalue_from_python_stage1_data *data) {
// 		void *storage =
// 			((converter::rvalue_from_python_storage<TfPyObjWrapper>*)data)
// 			->storage.bytes;
// 		// Make a TfPyObjWrapper holding the Python object.
// 		new (storage) TfPyObjWrapper(object(borrowed(obj_ptr)));
// 		data->convertible = storage;
// 	}
};

UsdPrim primFromSceneItem(const Ufe::SceneItem::Ptr& item)
{
	UsdSceneItem::Ptr usdItem = std::dynamic_pointer_cast<UsdSceneItem>(item);
	if (usdItem)
		return usdItem->prim();

	return UsdPrim();
}

BOOST_PYTHON_MODULE(ufe)
{
//	class_<UsdSceneItem, UsdSceneItem::Ptr, bases<Ufe::SceneItem>, boost::noncopyable >("UsdSceneItem", init<const Ufe::Path&, const UsdPrim&>())
// import mayaUsd.ufe
// # Error: RuntimeError: file <maya console> line 1: extension class wrapper for base class class Ufe_v0::SceneItem has not been created yet # 

	class_<UsdSceneItem, UsdSceneItem::Ptr, boost::noncopyable>("UsdSceneItem", init<const Ufe::Path&, const UsdPrim&>())
// print dir(mayaUsd.ufe)
// ['UsdSceneItem', '__doc__', '__file__', '__name__', '__package__', 'primFromSceneItem']
		.def("create", &UsdSceneItem::create)
		.staticmethod("create")
		.def("prim", &UsdSceneItem::prim, return_value_policy<copy_const_reference>())
		.def("nodeType", &UsdSceneItem::nodeType)
		.def("runTimeId", &UsdSceneItem::runTimeId)
		.def("path", &UsdSceneItem::path, return_value_policy<copy_const_reference>())
		.def("nodeType", &UsdSceneItem::nodeType)
		.def("isProperty", &UsdSceneItem::isProperty)
	;
// # Error: ArgumentError: file <maya console> line 4: Python argument types in
//     UsdSceneItem.__init__(UsdSceneItem, ufe.PyUfe.Path, Prim)
// did not match C++ signature:
//     __init__(struct _object * __ptr64, class Ufe_v0::Path, class pxrInternal_v0_19__pxrReserved__::UsdPrim) # 


	def("primFromSceneItem", primFromSceneItem);
// # Error: ArgumentError: file <maya console> line 1: Python argument types in
//     mayaUsd.ufe.primFromSceneItem(ufe.PyUfe.SceneItem)
// did not match C++ signature:
//    primFromSceneItem(class std::shared_ptr<class Ufe_v0::SceneItem>) # 

	// Register the from-python converter
	ufe_SceneItem::RegisterConversions();
}

#else

#include <Python.h>

static PyObject* helloworld(PyObject* self) {
   return Py_BuildValue("s", "Hello, Python extensions!!");
}

static char helloworld_docs[] =
   "helloworld( ): Any message you want to put here!!\n";

static PyObject *primFromSceneItem(PyObject *self, PyObject *args)
{
	PyObject* sceneItem = nullptr;

	if (PyArg_ParseTuple(args, "O", &sceneItem))
	{
//		if (PyObject_TypeCheck(sceneItem, &MPyMObject_Type::typeObj()))
		if (0 == std::strcmp(Py_TYPE(sceneItem)->tp_name, "ufe.PyUfe.SceneItem"))
		{
			Ufe::SceneItem::Ptr* item = (Ufe::SceneItem::Ptr*)sceneItem;
			if (item)
			{
				Ufe::Path path = (*item)->path();
				std::string p = path.string();
			}
		}
	}

	/* Do something interesting here. */
	Py_RETURN_NONE;
}

static PyMethodDef helloworld_funcs[] = {
   {"helloworld", (PyCFunction)helloworld, METH_NOARGS, helloworld_docs},
   {"primFromSceneItem", (PyCFunction)primFromSceneItem, METH_VARARGS, NULL },
   {NULL}
};


PyMODINIT_FUNC initufe()
{
	Py_InitModule3("ufe", helloworld_funcs,
					"Extension module example!");
}

#endif
