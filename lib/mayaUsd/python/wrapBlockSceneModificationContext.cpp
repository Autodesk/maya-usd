//
// Copyright 2018 Pixar
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
#include <mayaUsd/utils/blockSceneModificationContext.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_arg.hpp>

#include <memory>

using namespace boost::python;

namespace {

// This exposes BlockSceneModificationContext as a Python "context
// manager" object that can be used with the "with" statement.
class _PyBlockSceneModificationContext
{
public:
    void __enter__() { _context.reset(new MayaUsd::utils::BlockSceneModificationContext()); }

    void __exit__(object, object, object) { _context.reset(); }

private:
    std::shared_ptr<MayaUsd::utils::BlockSceneModificationContext> _context;
};

} // anonymous namespace

void wrapBlockSceneModificationContext()
{
    typedef _PyBlockSceneModificationContext Context;
    class_<Context>(
        "BlockSceneModificationContext",
        "Context manager for blocking scene modification status changes")
        .def("__enter__", &Context::__enter__, return_self<>())
        .def("__exit__", &Context::__exit__);
}
