//
// Copyright 2022 Autodesk
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

#include <mayaUsd/undo/OpUndoItemList.h>
#include <mayaUsd/undo/OpUndoItemRecorder.h>

#include <pxr/base/tf/diagnostic.h>
#include <pxr_python.h>

#include <memory>

using namespace PXR_BOOST_PYTHON_NAMESPACE;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

class PythonOpUndoItemList
{
public:
    PythonOpUndoItemList()
        : _undoItemList()
        , _recorder()
    {
    }

    ~PythonOpUndoItemList() { }

    void enter() { _recorder = std::make_unique<MayaUsd::OpUndoItemRecorder>(_undoItemList); }

    void exit(object, object, object) { _recorder.reset(); }

    bool isEmpty() { return _undoItemList.isEmpty(); }

    void clear() { _undoItemList.clear(); }

    void undo() { _undoItemList.undo(); }

    void redo() { _undoItemList.redo(); }

private:
    MayaUsd::OpUndoItemList                      _undoItemList;
    std::unique_ptr<MayaUsd::OpUndoItemRecorder> _recorder;
};

} // namespace

void wrapOpUndoItem()
{
    // OpUndoItemList
    {
        typedef PythonOpUndoItemList This;
        class_<This, PXR_BOOST_PYTHON_NAMESPACE::noncopyable>("OpUndoItemList", init<>())
            .def("__enter__", &This::enter)
            .def("__exit__", &This::exit)
            .def("isEmpty", &This::isEmpty)
            .def("clear", &This::clear)
            .def("undo", &This::undo)
            .def("redo", &This::redo);
    }
}
