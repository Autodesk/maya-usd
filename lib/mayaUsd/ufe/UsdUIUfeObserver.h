#ifndef USDUIUFEOBSERVER_H
#define USDUIUFEOBSERVER_H

//
// Copyright 2021 Autodesk
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

#include <mayaUsd/base/api.h>

#include <ufe/observer.h>

namespace MAYAUSD_NS_DEF {
namespace ufe {

//! \brief Helper class used to receive UFE notifications and respond to them by updating UI.
class UsdUIUfeObserver : public Ufe::Observer
{
public:
    UsdUIUfeObserver();

    // Delete the copy/move constructors assignment operators.
    UsdUIUfeObserver(const UsdUIUfeObserver&) = delete;
    UsdUIUfeObserver& operator=(const UsdUIUfeObserver&) = delete;
    UsdUIUfeObserver(UsdUIUfeObserver&&) = delete;
    UsdUIUfeObserver& operator=(UsdUIUfeObserver&&) = delete;

    //! Create/Destroy a UsdUIUfeObserver.
    static void create();
    static void destroy();

    void operator()(const Ufe::Notification& notification) override;

private:
    static Ufe::Observer::Ptr ufeObserver;
};

} // namespace ufe
} // namespace MAYAUSD_NS_DEF

#endif // USDUIUFEOBSERVER_H
