//
// Copyright 2024 Autodesk
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

#include <usdUfe/ufe/Global.h>
#include <usdUfe/ufe/UsdClipboardHandler.h>

#include <ufe/runTimeMgr.h>

#include <boost/python.hpp>
#include <boost/python/def.hpp>

using namespace boost::python;

void _setClipboardFileFormat(const std::string& formatTag)
{
    auto clipboardHandler = std::dynamic_pointer_cast<UsdUfe::UsdClipboardHandler>(
        Ufe::RunTimeMgr::instance().clipboardHandler(UsdUfe::getUsdRunTimeId()));
    if (clipboardHandler) {
        clipboardHandler->setClipboardFileFormat(formatTag);
    }
}

// clang-format off
void wrapClipboard()
{
    def("setClipboardFileFormat", _setClipboardFileFormat);
}
