//
// Copyright 2024 Autodesk, Inc. All rights reserved.
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

#include <mayaUsd/buildInfo.h>

namespace MAYAUSD_NS_DEF {

int         MayaUsdBuildInfo::buildNumber() { return MAYAUSD_BUILD_NUMBER; }
const char* MayaUsdBuildInfo::gitCommit() { return MAYAUSD_GIT_COMMIT; }
const char* MayaUsdBuildInfo::gitBranch() { return MAYAUSD_GIT_BRANCH; }
const char* MayaUsdBuildInfo::cutId() { return MAYAUSD_CUT_ID; }
const char* MayaUsdBuildInfo::buildDate() { return MAYAUSD_BUILD_DATE; }
bool        MayaUsdBuildInfo::buildAR()
{
#ifdef WANT_AR_BUILD
    return true;
#else
    return false;
#endif
}

} // namespace MAYAUSD_NS_DEF
