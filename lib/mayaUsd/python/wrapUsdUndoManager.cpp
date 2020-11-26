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

#include <mayaUsd/undo/UsdUndoBlock.h>
#include <mayaUsd/undo/UsdUndoManager.h>
#include <mayaUsd/undo/UsdUndoableItem.h>

#include <pxr/base/tf/pyContainerConversions.h>
#include <pxr/base/tf/pyNoticeWrapper.h>
#include <pxr/pxr.h>

#include <boost/python.hpp>

#include <memory>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
    class PythonUndoBlock
    {
    public:
        PythonUndoBlock()
            : _block(nullptr)
        {
        }

        ~PythonUndoBlock() { }

        void enter()
        {
            if (!TF_VERIFY(_block == nullptr)) {
                return;
            }
            _block = std::make_unique<MayaUsd::UsdUndoBlock>();
        }

        void exit(object, object, object)
        {
            if (!TF_VERIFY(_block != nullptr)) {
                return;
            }
            _block.reset();
        }

    private:
        std::unique_ptr<MayaUsd::UsdUndoBlock> _block;
    };

    void _trackLayerStates(const SdfLayerHandle& layer)
    {
        MayaUsd::UsdUndoManager::instance().trackLayerStates(layer);
    }

} // namespace 

void wrapUsdUndoManager()
{
    // UsdUndoManager
    {
        typedef MayaUsd::UsdUndoManager This;
        class_<This, boost::noncopyable>("UsdUndoManager", no_init)
            .def("trackLayerStates", &_trackLayerStates)
            .staticmethod("trackLayerStates");
    }

    // UsdUndoBlock
    {
        typedef PythonUndoBlock This;
        class_<This, boost::noncopyable>("UsdUndoBlock", init<>())
            .def("__enter__", &This::enter)
            .def("__exit__", &This::exit);
    }
}
