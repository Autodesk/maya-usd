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
#include "UfeNotifGuard.h"

#include <ufe/transform3d.h>

namespace {
Ufe::Path transform3dPath;
}

namespace MAYAUSD_NS_DEF {
namespace ufe {

InTransform3dChange::InTransform3dChange(const Ufe::Path& path) { transform3dPath = path; }

InTransform3dChange::~InTransform3dChange()
{
    Ufe::Transform3d::notify(transform3dPath);
    transform3dPath = Ufe::Path();
}

/* static */
bool InTransform3dChange::inTransform3dChange() { return !transform3dPath.empty(); }

} // namespace ufe
} // namespace MAYAUSD_NS_DEF
