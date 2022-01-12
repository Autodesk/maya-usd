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

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

#include <memory>

using namespace boost::python;

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

    void enter()
    {
        if (!TF_VERIFY(_undoItemList == nullptr)) {
            return;
        }
        _undoItemList = std::make_unique<MayaUsd::OpUndoItemList>();
        _recorder = std::make_unique<MayaUsd::OpUndoItemRecorder>(*_undoItemList);
    }

    void exit(object, object, object)
    {
        if (!TF_VERIFY(_undoItemList != nullptr)) {
            return;
        }
        _recorder.reset();
        _undoItemList.reset();
    }

private:
    std::unique_ptr<MayaUsd::OpUndoItemList>     _undoItemList;
    std::unique_ptr<MayaUsd::OpUndoItemRecorder> _recorder;
};

} // namespace

void wrapOpUndoItem()
{
    // OpUndoItemList
    {
        typedef PythonOpUndoItemList This;
        class_<This, boost::noncopyable>("OpUndoItemList", init<>())
            .def("__enter__", &This::enter)
            .def("__exit__", &This::exit);
    }
}
