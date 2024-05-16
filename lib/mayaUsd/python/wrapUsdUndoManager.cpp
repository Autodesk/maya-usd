//
// Copyright 2020 Autodesk
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

#include <mayaUsd/undo/MayaUsdUndoBlock.h>

#include <usdUfe/undo/UsdUndoManager.h>
#include <usdUfe/undo/UsdUndoableItem.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyNoticeWrapper.h>
#include <pxr/pxr.h>
#include <pxr/usd/sdf/layer.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
class PythonUndoBlock
{
public:
    PythonUndoBlock(UsdUfe::UsdUndoableItem& item)
        : _item(&item)
        , _block(nullptr)
    {
    }

    PythonUndoBlock()
        : _item(nullptr)
        , _block(nullptr)
    {
    }

    ~PythonUndoBlock() { }

    void enter()
    {
        if (!TF_VERIFY(_block == nullptr)) {
            return;
        }
        if (_item)
            _block = std::make_unique<UsdUfe::UsdUndoBlock>(_item);
        else
            _block = std::make_unique<MayaUsd::MayaUsdUndoBlock>();
    }

    void exit(object, object, object) { _block.reset(); }

private:
    UsdUfe::UsdUndoableItem*              _item;
    std::unique_ptr<UsdUfe::UsdUndoBlock> _block;
};

void _trackLayerStates(const SdfLayerHandle& layer)
{
    UsdUfe::UsdUndoManager::instance().trackLayerStates(layer);
}

} // namespace

void wrapUsdUndoManager()
{
    // UsdUndoManager
    {
        typedef UsdUfe::UsdUndoManager This;
        class_<This, boost::noncopyable>("UsdUndoManager", no_init)
            .def("trackLayerStates", &_trackLayerStates)
            .staticmethod("trackLayerStates");
    }

    // UsdUfe::UsdUndoableItem
    {
        class_<UsdUfe::UsdUndoableItem>("UsdUndoableItem")
            .def("undo", &UsdUfe::UsdUndoableItem::undo)
            .def("redo", &UsdUfe::UsdUndoableItem::redo);
    }

    // UsdUndoBlock
    {
        typedef PythonUndoBlock This;
        class_<This, boost::noncopyable>("UsdUndoBlock")
            .def(init<>())
            .def(init<UsdUfe::UsdUndoableItem&>(arg("item")))
            .def("__enter__", &This::enter)
            .def("__exit__", &This::exit);
    }
}
